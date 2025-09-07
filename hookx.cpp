#include <windows.h>
#include <xinput.h>
#include <detours/detours.h>
#include <fmt/core.h>
#include <cmath>

// Original XInputGetState function pointer
static DWORD(WINAPI* TrueXInputGetState)(DWORD dwUserIndex, XINPUT_STATE* pState) = XInputGetState;

// Global variables for auto controller simulation
static bool g_enableAutoController = true;
static DWORD g_frameCounter = 0;
static float g_stickRadius = 0.8f;  // How far from center to move the sticks (0.0 to 1.0)
static float g_rotationSpeed = 0.02f;  // How fast to rotate the sticks

// Convert float stick values (-1.0 to 1.0) to SHORT values (-32768 to 32767)
SHORT FloatToStick(float value) {
    if (value > 1.0f) value = 1.0f;
    if (value < -1.0f) value = -1.0f;
    return static_cast<SHORT>(value * 32767.0f);
}

// Generate circular stick movement
void GenerateCircularStickMovement(XINPUT_GAMEPAD& gamepad) {
    float angle = g_frameCounter * g_rotationSpeed;
    
    // Left stick - circular movement
    float leftX = cos(angle) * g_stickRadius;
    float leftY = sin(angle) * g_stickRadius;
    
    // Right stick - circular movement in opposite direction
    float rightX = cos(-angle * 0.7f) * g_stickRadius;
    float rightY = sin(-angle * 0.7f) * g_stickRadius;
    
    gamepad.sThumbLX = FloatToStick(leftX);
    gamepad.sThumbLY = FloatToStick(leftY);
    gamepad.sThumbRX = FloatToStick(rightX);
    gamepad.sThumbRY = FloatToStick(rightY);
}

// Hooked XInputGetState function
DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    // Call the original function first
    DWORD result = TrueXInputGetState(dwUserIndex, pState);
    
    // If the controller is not connected, simulate one
    if (result == ERROR_DEVICE_NOT_CONNECTED && g_enableAutoController && dwUserIndex == 0) {
        fmt::print("Simulating controller {} - Frame {}\n", dwUserIndex, g_frameCounter);
        
        // Initialize a fake controller state
        ZeroMemory(pState, sizeof(XINPUT_STATE));
        pState->dwPacketNumber = g_frameCounter;
        
        // Generate automatic stick movements
        GenerateCircularStickMovement(pState->Gamepad);
        
        // Set some buttons (optional)
        if ((g_frameCounter / 120) % 2 == 0) {  // Toggle every 2 seconds at 60fps
            pState->Gamepad.wButtons |= XINPUT_GAMEPAD_A;
        }
        
        g_frameCounter++;
        return ERROR_SUCCESS;  // Report success (controller connected)
    }
    
    // If a real controller is connected, optionally modify its input
    if (result == ERROR_SUCCESS && g_enableAutoController) {
        fmt::print("Real controller {} detected, enhancing input - Frame {}\n", dwUserIndex, g_frameCounter);
        
        // You can modify real controller input here
        // For example, add auto-movement to existing controller input:
        float angle = g_frameCounter * g_rotationSpeed * 0.5f;
        float enhanceX = cos(angle) * 0.3f;  // Smaller enhancement
        float enhanceY = sin(angle) * 0.3f;
        
        // Add enhancement to existing input (clamp to valid range)
        int newLX = pState->Gamepad.sThumbLX + FloatToStick(enhanceX);
        int newLY = pState->Gamepad.sThumbLY + FloatToStick(enhanceY);
        
        pState->Gamepad.sThumbLX = static_cast<SHORT>(max(-32768, min(32767, newLX)));
        pState->Gamepad.sThumbLY = static_cast<SHORT>(max(-32768, min(32767, newLY)));
        
        g_frameCounter++;
    }
    
    return result;
}

// Console control handler for graceful shutdown
BOOL WINAPI ConsoleHandler(DWORD dwType) {
    switch (dwType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
        fmt::print("Shutting down HookX...\n");
        
        // Detach hooks
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)TrueXInputGetState, HookedXInputGetState);
        DetourTransactionCommit();
        
        fmt::print("Hooks removed successfully.\n");
        return TRUE;
    }
    return FALSE;
}

int main() {
    fmt::print("HookX - XInput Controller Hook\n");
    fmt::print("===============================\n");
    fmt::print("This tool hooks XInput1_4.dll to provide automatic controller input.\n");
    fmt::print("Press Ctrl+C to exit gracefully.\n\n");
    
    // Set up console handler for graceful shutdown
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    // Initialize Detours
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    
    // Hook XInputGetState
    LONG error = DetourAttach(&(PVOID&)TrueXInputGetState, HookedXInputGetState);
    if (error != NO_ERROR) {
        fmt::print("ERROR: Failed to attach XInputGetState hook. Error code: {}\n", error);
        return 1;
    }
    
    // Commit the transaction
    error = DetourTransactionCommit();
    if (error != NO_ERROR) {
        fmt::print("ERROR: Failed to commit detour transaction. Error code: {}\n", error);
        return 1;
    }
    
    fmt::print("SUCCESS: XInputGetState hook installed!\n");
    fmt::print("Auto-controller enabled: {}\n", g_enableAutoController ? "YES" : "NO");
    fmt::print("Stick movement radius: {:.2f}\n", g_stickRadius);
    fmt::print("Rotation speed: {:.3f}\n\n", g_rotationSpeed);
    
    fmt::print("Hook is active. The program will:\n");
    fmt::print("1. Simulate a controller if none is connected\n");
    fmt::print("2. Enhance real controller input if one is connected\n");
    fmt::print("3. Generate circular stick movements automatically\n\n");
    
    // Keep the program running to maintain hooks
    fmt::print("Monitoring XInput calls... (Press Ctrl+C to exit)\n");
    
    // Main loop - just sleep and occasionally print status
    DWORD statusCounter = 0;
    while (true) {
        Sleep(1000);  // Sleep for 1 second
        
        statusCounter++;
        if (statusCounter % 10 == 0) {  // Print status every 10 seconds
            fmt::print("Status: Hook active for {} seconds, Frame count: {}\n", 
                      statusCounter, g_frameCounter);
        }
    }
    
    return 0;
}