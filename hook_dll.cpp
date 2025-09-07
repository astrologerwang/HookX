#include <windows.h>
#include <xinput.h>
#include <detours/detours.h>
#include <fstream>
#include <cmath>

// Original function pointer
static DWORD(WINAPI* TrueXInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState) = XInputGetState;

// Global variables
static bool g_enableAutoController = true;
static DWORD g_frameCounter = 0;
static float g_stickRadius = 0.8f;
static float g_rotationSpeed = 0.02f;

// Logging function
void LogMessage(const char* message) {
    std::ofstream log("C:\\temp\\hookx_log.txt", std::ios::app);
    if (log.is_open()) {
        log << message << std::endl;
        log.close();
    }
}

SHORT FloatToStick(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    return static_cast<SHORT>(value * 32767.0f);
}

void GenerateCircularStickMovement(XINPUT_GAMEPAD& gamepad) {
    float angle = g_frameCounter * g_rotationSpeed;
    
    float leftX = cos(angle) * g_stickRadius;
    float leftY = sin(angle) * g_stickRadius;
    float rightX = cos(-angle * 0.7f) * g_stickRadius;
    float rightY = sin(-angle * 0.7f) * g_stickRadius;
    
    gamepad.sThumbLX = FloatToStick(leftX);
    gamepad.sThumbLY = FloatToStick(leftY);
    gamepad.sThumbRX = FloatToStick(rightX);
    gamepad.sThumbRY = FloatToStick(rightY);
}

DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    char logBuffer[256];
    sprintf_s(logBuffer, "XInputGetState called for controller %d", dwUserIndex);
    LogMessage(logBuffer);
    
    DWORD result = TrueXInputGetState(dwUserIndex, pState);
    
    // Force simulation mode - always override with simulated input
    if (g_enableAutoController && dwUserIndex == 0) {
        sprintf_s(logBuffer, "Force simulating controller %d - Frame %d (Real controller: %s)", 
                 dwUserIndex, g_frameCounter, (result == ERROR_SUCCESS) ? "Connected" : "Not connected");
        LogMessage(logBuffer);
        
        ZeroMemory(pState, sizeof(XINPUT_STATE));
        pState->dwPacketNumber = g_frameCounter;
        
        GenerateCircularStickMovement(pState->Gamepad);
        
        // if ((g_frameCounter / 120) % 2 == 0) {
        //     pState->Gamepad.wButtons |= XINPUT_GAMEPAD_A;
        // }
        
        g_frameCounter++;
        return ERROR_SUCCESS;
    }
    
    if (result == ERROR_SUCCESS && g_enableAutoController) {
        sprintf_s(logBuffer, "Real controller %d detected, enhancing input - Frame %d", dwUserIndex, g_frameCounter);
        LogMessage(logBuffer);
        
        // Add stronger enhancement to existing controller input
        float angle = g_frameCounter * g_rotationSpeed * 0.5f;
        float enhanceX = cos(angle) * 0.8f;  // Increased from 0.3f to 0.8f
        float enhanceY = sin(angle) * 0.8f;  // Increased from 0.3f to 0.8f
        
        int newLX = pState->Gamepad.sThumbLX + FloatToStick(enhanceX);
        int newLY = pState->Gamepad.sThumbLY + FloatToStick(enhanceY);
        
        pState->Gamepad.sThumbLX = static_cast<SHORT>(max(-32768, min(32767, newLX)));
        pState->Gamepad.sThumbLY = static_cast<SHORT>(max(-32768, min(32767, newLY)));
        
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
