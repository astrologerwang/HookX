#include <windows.h>
#include <xinput.h>
#include <detours/detours.h>
#include <fstream>
#include <cmath>
#include <string>
#include <exception>

// Original function pointer
static DWORD(WINAPI* TrueXInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState) = XInputGetState;

static bool g_enableAutoController = true;
static DWORD g_frameCounter = 0;
static float g_stickRadius = 0.8f;
static float g_rotationSpeed = 0.02f;
static void* g_cameraForward = nullptr;
static void* g_position = nullptr;
static bool g_rotatingCamera = false;
static bool g_lastYButtonState = false;

static float g_positionX = 0.0f;
static float g_positionY = 0.0f;
static float g_positionZ = 0.0f;

static float g_cameraForwardX = 0.0f;
static float g_cameraForwardY = 0.0f;
static float g_cameraForwardZ = 0.0f;

static float g_centerX = 12.6f;
static float g_centerY = 0.0f;
static float g_centerZ = -15.0f;

// Timing for periodic logging
static DWORD g_lastLogTime = 0;
static DWORD g_lastAddressCheckTime = 0;
static const DWORD LOG_INTERVAL_MS = 1000; // 1 second
static const DWORD ADDRESS_CHECK_INTERVAL_MS = 500; // Check for address updates every 500ms

// Logging function
void LogMessage(const char* message) {
    std::ofstream log("D:\\temp\\hookx_log.txt", std::ios::app);
    if (log.is_open()) {
        log << message << std::endl;
        log.close();
    }
}

// Function to safely log g_cameraForward value
void LogCameraForwardValue() {
    char logBuffer[512];

    if (g_cameraForward == nullptr) {
        sprintf_s(logBuffer, "[CAMERA LOG] g_cameraForward is nullptr");
        LogMessage(logBuffer);
        return;
    }

    // Try to safely read the value pointed to by g_cameraForward
    __try {
        // Assuming g_cameraForward points to a float (or float array)
        float* cameraPtr = static_cast<float*>(g_cameraForward);
        sprintf_s(logBuffer, "[CAMERA LOG] g_cameraForward: 0x%p -> Value: %.6f",
            g_cameraForward, *cameraPtr);
        LogMessage(logBuffer);

        // If it's a 3D vector (X, Y, Z), log all three values
        sprintf_s(logBuffer, "[CAMERA LOG] Camera Forward Vector: X=%.6f, Y=%.6f, Z=%.6f",
            cameraPtr[0], cameraPtr[1], cameraPtr[2]);
        LogMessage(logBuffer);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        sprintf_s(logBuffer, "[CAMERA LOG] g_cameraForward: 0x%p -> Cannot read value (Access Violation)",
            g_cameraForward);
        LogMessage(logBuffer);
    }
}

// Function to safely log g_position value
void LogPositionValue() {
    char logBuffer[512];

    if (g_position == nullptr) {
        sprintf_s(logBuffer, "[POSITION LOG] g_position is nullptr");
        LogMessage(logBuffer);
        return;
    }

    // Try to safely read the value pointed to by g_position
    __try {
        // Assuming g_position points to a float array (X, Y, Z)
        float* positionPtr = static_cast<float*>(g_position);
        sprintf_s(logBuffer, "[POSITION LOG] g_position: 0x%p -> Position: X=%.6f, Y=%.6f, Z=%.6f",
            g_position, positionPtr[0], positionPtr[1], positionPtr[2]);
        LogMessage(logBuffer);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        sprintf_s(logBuffer, "[POSITION LOG] g_position: 0x%p -> Cannot read value (Access Violation)",
            g_position);
        LogMessage(logBuffer);
    }
}

// Function to check for address updates from hookx.exe
void CheckForAddressUpdates() {
    std::ifstream addressFile("D:\\temp\\hookx_addresses.txt");
    if (!addressFile.is_open()) {
        return; // File doesn't exist yet, that's okay
    }

    std::string line;
    while (std::getline(addressFile, line)) {
        if (line.find("g_cameraForward=") == 0) {
            std::string addressStr = line.substr(16); // Skip "g_cameraForward="
            try {
                uintptr_t newAddress = std::stoull(addressStr, nullptr, 16);
                void* newPtr = reinterpret_cast<void*>(newAddress);

                if (newPtr != g_cameraForward) {
                    g_cameraForward = newPtr;
                    // Automatically set g_position as g_cameraForward + 16 bytes
                    g_position = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(g_cameraForward) + 16);

                    char logBuffer[256];
                    sprintf_s(logBuffer, "[ADDRESS UPDATE] g_cameraForward set to: 0x%p", g_cameraForward);
                    LogMessage(logBuffer);
                    sprintf_s(logBuffer, "[ADDRESS UPDATE] g_position auto-set to: 0x%p (g_cameraForward + 16)", g_position);
                    LogMessage(logBuffer);
                }
            } catch (const std::exception&) {
                LogMessage("[ADDRESS UPDATE] Failed to parse g_cameraForward address");
            }
        }
        // Remove the g_position parsing since it's now calculated automatically
    }
    addressFile.close();
}

SHORT FloatToStick(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    return static_cast<SHORT>(value * 32767.0f);
}

void GenerateCircularStickMovement(XINPUT_GAMEPAD& gamepad) {
    if (g_rotatingCamera) {
        gamepad.sThumbLX = 0;
        gamepad.sThumbLY = 0;
        gamepad.sThumbRX = FloatToStick(1.0f);
        gamepad.sThumbRY = 0;
    }
}

DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    char logBuffer[256];
    sprintf_s(logBuffer, "XInputGetState called for controller %d (g_cameraForward: 0x%p", dwUserIndex, g_cameraForward);
    LogMessage(logBuffer);

    DWORD currentTime = GetTickCount();

    // Periodic logging of g_cameraForward value (every 1 second)
    if (currentTime - g_lastLogTime >= LOG_INTERVAL_MS) {
        LogCameraForwardValue();
        LogPositionValue();
        g_lastLogTime = currentTime;
    }

    // Check for address updates from hookx.exe (every 500ms)
    if (currentTime - g_lastAddressCheckTime >= ADDRESS_CHECK_INTERVAL_MS) {
        CheckForAddressUpdates();
        g_lastAddressCheckTime = currentTime;
    }

    DWORD result = TrueXInputGetState(dwUserIndex, pState);

    if (result == ERROR_SUCCESS && dwUserIndex == 0) {
        bool currentYButtonState = (pState->Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;

        if (currentYButtonState && !g_lastYButtonState) {
            g_rotatingCamera = !g_rotatingCamera;
            sprintf_s(logBuffer, "Y button pressed! Continuous leftY input: %s",
                g_rotatingCamera ? "ENABLED" : "DISABLED");
            LogMessage(logBuffer);
        }

        g_lastYButtonState = currentYButtonState;
    }

    if (g_enableAutoController && dwUserIndex == 0) {
        sprintf_s(logBuffer, "Force simulating controller %d - Frame %d (Real controller: %s, Continuous leftY: %s)",
            dwUserIndex, g_frameCounter, (result == ERROR_SUCCESS) ? "Connected" : "Not connected",
            g_rotatingCamera ? "ON" : "OFF");
        LogMessage(logBuffer);

        ZeroMemory(pState, sizeof(XINPUT_STATE));
        pState->dwPacketNumber = g_frameCounter;

        GenerateCircularStickMovement(pState->Gamepad);

        g_frameCounter++;
        return ERROR_SUCCESS;
    }

    if (result == ERROR_SUCCESS && g_enableAutoController) {
        sprintf_s(logBuffer, "Real controller %d detected, enhancing input - Frame %d (Continuous leftY: %s)",
            dwUserIndex, g_frameCounter, g_rotatingCamera ? "ON" : "OFF");
        LogMessage(logBuffer);

        if (g_rotatingCamera) {

            pState->Gamepad.sThumbLX = 0;
            pState->Gamepad.sThumbLY = FloatToStick(1.0f);
        } else {
            float angle = g_frameCounter * g_rotationSpeed * 0.5f;
            float enhanceX = cos(angle) * 0.8f;
            float enhanceY = sin(angle) * 0.8f;

            int newLX = pState->Gamepad.sThumbLX + FloatToStick(enhanceX);
            int newLY = pState->Gamepad.sThumbLY + FloatToStick(enhanceY);

            pState->Gamepad.sThumbLX = static_cast<SHORT>(max(-32768, min(32767, newLX)));
            pState->Gamepad.sThumbLY = static_cast<SHORT>(max(-32768, min(32767, newLY)));
        }

        g_frameCounter++;
    }

    return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        LogMessage("Hook DLL attached to process - HookX started");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueXInputGetState, HookedXInputGetState);
        if (DetourTransactionCommit() == NO_ERROR) {
            LogMessage("XInputGetState hook installed successfully");
        } else {
            LogMessage("Failed to install XInputGetState hook");
        }
        break;

    case DLL_PROCESS_DETACH:
        LogMessage("Hook DLL detaching from process - HookX stopping");
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueXInputGetState, HookedXInputGetState);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}
