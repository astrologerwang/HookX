#include <windows.h>
#include <iostream>

// Custom message ID for broadcast communication (must match hook_dll.cpp)
const UINT WM_HOOKX_BROADCAST = WM_USER + 1000;

int main()
{
    std::cout << "HookX Broadcast Message Sender\n";
    std::cout << "===============================\n";

    // Example data to send
    int exampleData = 12345;
    void *dataPointer = &exampleData;

    std::cout << "Sending broadcast message with pointer: 0x" << std::hex << dataPointer << std::endl;
    // std::cout << "Data value: " << std::dec << exampleData << std::endl;

    // Send broadcast message to all top-level windows
    LRESULT result = SendMessage(HWND_BROADCAST, WM_HOOKX_BROADCAST, 0, reinterpret_cast<LPARAM>(dataPointer));

    std::cout << "Broadcast message sent. Result: " << result << std::endl;
    std::cout << "Check C:\\temp\\hookx_log.txt to see if the hook received the message." << std::endl;

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}
