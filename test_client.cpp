#include <windows.h>
#include <xinput.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

#pragma comment(lib, "xinput.lib")

void PrintControllerState(DWORD controller, const XINPUT_STATE& state) {
    const auto& gp = state.Gamepad;
    
    std::cout << "Controller " << controller << " (Packet: " << state.dwPacketNumber << "):\n";
    std::cout << "  Left Stick:  X=" << std::setw(6) << gp.sThumbLX << ", Y=" << std::setw(6) << gp.sThumbLY << "\n";
    std::cout << "  Right Stick: X=" << std::setw(6) << gp.sThumbRX << ", Y=" << std::setw(6) << gp.sThumbRY << "\n";
    std::cout << "  Buttons: " << std::hex << "0x" << gp.wButtons << std::dec << "\n";
    std::cout << "  Triggers: L=" << static_cast<int>(gp.bLeftTrigger) << ", R=" << static_cast<int>(gp.bRightTrigger) << "\n\n";
}

int main() {
    std::cout << "XInput Test Client\n";
    std::cout << "==================\n";
    std::cout << "This program will poll XInput controllers to test the hook.\n";
    std::cout << "Press Ctrl+C to exit.\n\n";
    
    int frame = 0;
    while (true) {
        std::cout << "=== Frame " << ++frame << " ===\n";
        
        for (DWORD i = 0; i < 4; ++i) {
            XINPUT_STATE state;
            DWORD result = XInputGetState(i, &state);
            
            if (result == ERROR_SUCCESS) {
                PrintControllerState(i, state);
            } else {
                std::cout << "Controller " << i << ": Not connected (Error: " << result << ")\n\n";
            }
        }
        
        std::cout << "-------------------\n\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 2 FPS for easy reading
    }
    
    return 0;
}
