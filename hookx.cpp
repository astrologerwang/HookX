#include <windows.h>
#include <tlhelp32.h>
#include <fmt/core.h>
#include <string>
#include <filesystem>
#include <iostream>

DWORD GetProcessIdByName(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (processName == pe32.szExeFile) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD processId, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        fmt::print("Failed to open process. Error: {}\n", GetLastError());
        return false;
    }
    
    // Allocate memory in target process
    LPVOID pDllPath = VirtualAllocEx(hProcess, 0, dllPath.size() + 1, 
                                     MEM_COMMIT, PAGE_READWRITE);
    if (!pDllPath) {
        fmt::print("Failed to allocate memory in target process\n");
        CloseHandle(hProcess);
        return false;
    }
    
    // Write DLL path to target process
    WriteProcessMemory(hProcess, pDllPath, dllPath.c_str(), dllPath.size() + 1, 0);
    
    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, 
                                        (LPTHREAD_START_ROUTINE)LoadLibraryA, 
                                        pDllPath, 0, 0);
    if (!hThread) {
        fmt::print("Failed to create remote thread. Error: {}\n", GetLastError());
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    
    // Wait for injection to complete
    WaitForSingleObject(hThread, INFINITE);
    
    // Check if LoadLibrary succeeded
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    
    // Cleanup
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    
    return exitCode != 0;  // LoadLibrary returns module handle (non-zero) on success
}

int main() {
    fmt::print("HookX - XInput Controller Hook for Elden Ring\n");
    fmt::print("=============================================\n");
    fmt::print("This tool injects a hook DLL into Elden Ring to provide automatic controller input.\n\n");
    
    // Create temp directory for logs
    CreateDirectoryA("C:\\temp", NULL);
    
    // Get the current executable directory
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    
    // Build DLL path
    std::string dllPath = (exeDir / "hook_dll.dll").string();
    
    fmt::print("Looking for hook DLL at: {}\n", dllPath);
    
    // Check if DLL exists
    if (!std::filesystem::exists(dllPath)) {
        fmt::print("ERROR: hook_dll.dll not found!\n");
        fmt::print("Make sure to build the project first.\n");
        fmt::print("Press Enter to exit...\n");
        std::cin.get();
        return 1;
    }
    
    fmt::print("Looking for Elden Ring process (eldenring.exe)...\n");
    
    DWORD processId = 0;
    int attempts = 0;
    const int maxAttempts = 30; // Try for 30 seconds
    
    while (processId == 0 && attempts < maxAttempts) {
        processId = GetProcessIdByName(L"eldenring.exe");
        if (processId == 0) {
            fmt::print("Elden Ring not found, waiting... ({}/{})\n", attempts + 1, maxAttempts);
            Sleep(1000);
            attempts++;
        }
    }
    
    if (processId == 0) {
        fmt::print("ERROR: Elden Ring process not found after {} seconds!\n", maxAttempts);
        fmt::print("Make sure Elden Ring is running.\n");
        fmt::print("Press Enter to exit...\n");
        std::cin.get();
        return 1;
    }
    
    fmt::print("Found Elden Ring process with PID: {}\n", processId);
    fmt::print("Attempting to inject hook DLL...\n");
    
    if (InjectDLL(processId, dllPath)) {
        fmt::print("SUCCESS: Hook DLL injected into Elden Ring!\n\n");
        fmt::print("Hook is now active. The system will:\n");
        fmt::print("1. Simulate a controller if none is connected\n");
        fmt::print("2. Enhance real controller input if one is connected\n");
        fmt::print("3. Generate circular stick movements automatically\n\n");
        fmt::print("Check C:\\temp\\hookx_log.txt for hook activity.\n\n");
        fmt::print("Press Enter to exit (this will NOT remove the hook)...\n");
    } else {
        fmt::print("ERROR: Failed to inject DLL into Elden Ring!\n");
        fmt::print("Make sure you're running as administrator.\n");
        fmt::print("Press Enter to exit...\n");
    }
    
    std::cin.get();
    return 0;
}