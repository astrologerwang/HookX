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
static float g_rotationSpeed = 0.02f;
static void* g_cameraForward = nullptr;
static bool g_rotatingCamera = false;
static bool g_lastYButtonState = false;

static float g_centerX = 12.5f;
static float g_centerY = 0.0f;
static float g_centerZ = 16.83f;

static float g_original_thumb_L_X = 0.6f;
static float g_original_thumb_L_Y = 0.0f;
static float g_original_thumb_R_X = -0.3f;
static float g_original_thumb_R_Y = 0.0f;

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

// Function to check for address updates from hookx.exe
void CheckForAddressUpdates() {
    std::ifstream addressFile("D:\\temp\\hookx_addresses.txt");
    if (!addressFile.is_open()) {
        static bool hasLoggedFileNotFound = false;
        if (!hasLoggedFileNotFound) {
            LogMessage("[ADDRESS CHECK] Address file not found: D:\\temp\\hookx_addresses.txt");
            hasLoggedFileNotFound = true;
        }
        return; // File doesn't exist yet, that's okay
    }

    std::string line;
    bool foundAddress = false;
    while (std::getline(addressFile, line)) {
        if (line.find("g_cameraForward=") == 0) {
            std::string addressStr = line.substr(16); // Skip "g_cameraForward="
            try {
                uintptr_t newAddress = std::stoull(addressStr, nullptr, 16);
                void* newPtr = reinterpret_cast<void*>(newAddress);

                if (newAddress == 0) {
                    static bool hasLoggedZeroAddress = false;
                    if (!hasLoggedZeroAddress) {
                        LogMessage("[ADDRESS CHECK] Found zero address in file - please update with real address");
                        hasLoggedZeroAddress = true;
                    }
                    foundAddress = true;
                    continue;
                }

                if (newPtr != g_cameraForward) {
                    g_cameraForward = newPtr;
                    char logBuffer[256];
                    sprintf_s(logBuffer, "[ADDRESS UPDATE] g_cameraForward auto-loaded from file: 0x%p", g_cameraForward);
                    LogMessage(logBuffer);
                }
                foundAddress = true;
            } catch (const std::exception&) {
                LogMessage("[ADDRESS UPDATE] Failed to parse g_cameraForward address from file");
            }
        }
    }

    if (!foundAddress) {
        static bool hasLoggedNoAddress = false;
        if (!hasLoggedNoAddress) {
            LogMessage("[ADDRESS CHECK] No g_cameraForward address found in file");
            hasLoggedNoAddress = true;
        }
    }

    addressFile.close();
}

SHORT FloatToStick(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    return static_cast<SHORT>(value * 32767.0f);
}

void GenerateCircularMovement(XINPUT_GAMEPAD& gamepad) {

    if (g_cameraForward == nullptr) {
        return;
    }

    float thumb_L_X = g_original_thumb_L_X;
    float thumb_L_Y = g_original_thumb_L_Y;
    float thumb_R_X = g_original_thumb_R_X;
    float thumb_R_Y = g_original_thumb_R_Y;

    float* cameraPtr = static_cast<float*>(g_cameraForward);
    float forwardX = cameraPtr[0];
    float forwardY = cameraPtr[1];
    float forwardZ = cameraPtr[2];
    float posX = cameraPtr[4];
    float posY = cameraPtr[5];
    float posZ = cameraPtr[6];

    float curForwardX = posX - g_centerX;
    float curForwardZ = posZ - g_centerZ;
    float dot = (curForwardX * forwardZ) - (curForwardZ * forwardX);
    float distance = sqrt(curForwardX * curForwardX + curForwardZ * curForwardZ);

    if (g_rotatingCamera) {

        thumb_R_X = g_original_thumb_R_X - 0.01f * dot;

        gamepad.sThumbLX = FloatToStick(thumb_L_X);
        gamepad.sThumbLY = FloatToStick(thumb_L_Y);
        gamepad.sThumbRX = FloatToStick(thumb_R_X);
        gamepad.sThumbRY = FloatToStick(thumb_R_Y);
    }
}

DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {

    DWORD currentTime = GetTickCount();

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
        }

        g_lastYButtonState = currentYButtonState;
    }

    if (g_enableAutoController && dwUserIndex == 0) {

        ZeroMemory(pState, sizeof(XINPUT_STATE));
        pState->dwPacketNumber = g_frameCounter;

        GenerateCircularMovement(pState->Gamepad);

        g_frameCounter++;
        return ERROR_SUCCESS;
    }

    if (result == ERROR_SUCCESS && g_enableAutoController) {
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
