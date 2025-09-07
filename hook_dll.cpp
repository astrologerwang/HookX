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
static void* g_fieldArea = nullptr;  // Raw pointer to receive broadcast messages

// Custom message ID for broadcast communication
static const UINT WM_HOOKX_BROADCAST = WM_USER + 1000;

// Logging function
void LogMessage(const char* message) {
    std::ofstream log("D:\\temp\\hookx_log.txt", std::ios::app);
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
    sprintf_s(logBuffer, "XInputGetState called for controller %d (g_fieldArea: 0x%p)", dwUserIndex, g_fieldArea);
    LogMessage(logBuffer);

    sprintf_s(logBuffer, "Message id: %u", WM_HOOKX_BROADCAST);
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

// Window procedure to handle broadcast messages
LRESULT CALLBACK BroadcastWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_HOOKX_BROADCAST) {
        // Receive the pointer from another application
        g_fieldArea = reinterpret_cast<void*>(lParam);
        
        char logBuffer[256];
        sprintf_s(logBuffer, "Received broadcast message: g_fieldArea set to 0x%p", g_fieldArea);
        LogMessage(logBuffer);
        
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Global variables for message window
static HWND g_messageWindow = nullptr;
static const char* WINDOW_CLASS_NAME = "HookXMessageWindow";

// Create a hidden window to receive broadcast messages
bool CreateMessageWindow() {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = BroadcastWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    if (!RegisterClassA(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            char logBuffer[256];
            sprintf_s(logBuffer, "Failed to register window class. Error: %d", error);
            LogMessage(logBuffer);
            return false;
        }
    }
    
    g_messageWindow = CreateWindowA(
        WINDOW_CLASS_NAME,
        "HookX Message Window",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,  // Message-only window
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (!g_messageWindow) {
        char logBuffer[256];
        sprintf_s(logBuffer, "Failed to create message window. Error: %d", GetLastError());
        LogMessage(logBuffer);
        return false;
    }
    
    LogMessage("Message window created successfully for broadcast communication");
    return true;
}

// Cleanup message window
void DestroyMessageWindow() {
    if (g_messageWindow) {
        DestroyWindow(g_messageWindow);
        g_messageWindow = nullptr;
        LogMessage("Message window destroyed");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        LogMessage("Hook DLL attached to process - HookX started");
        
        // Create message window for broadcast communication
        if (!CreateMessageWindow()) {
            LogMessage("Warning: Failed to create message window for broadcast communication");
        }
        
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
        
        // Cleanup message window
        DestroyMessageWindow();
        
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueXInputGetState, HookedXInputGetState);
        DetourTransactionCommit();
        break;
    }
    return TRUE;
}
