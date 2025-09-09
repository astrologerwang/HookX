#include <windows.h>
#include <tlhelp32.h>
#include <fmt/core.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>

DWORD GetProcessIdByName(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

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

    return exitCode != 0; // LoadLibrary returns module handle (non-zero) on success
}

// Function to write addresses to communication file
void WriteAddressesToFile(uintptr_t cameraForwardAddr) {
    std::ofstream addressFile("D:\\temp\\hookx_addresses.txt");
    if (addressFile.is_open()) {
        if (cameraForwardAddr != 0) {
            addressFile << "g_cameraForward=" << std::hex << cameraForwardAddr << std::endl;
        }
        addressFile.close();

        if (cameraForwardAddr != 0) {
            fmt::print("Address written to communication file:\n");
            fmt::print("  g_cameraForward: 0x{:X}\n", cameraForwardAddr);
            fmt::print("  g_position will be auto-set to: 0x{:X} (g_cameraForward + 16)\n", cameraForwardAddr + 16);
        }
    } else {
        fmt::print("ERROR: Could not write address to communication file!\n");
    }
}

int main() {
    fmt::print("HookX - XInput Controller Hook for Elden Ring\n");
    fmt::print("=============================================\n");
    fmt::print("This tool injects a hook DLL into Elden Ring to provide automatic controller input.\n\n");

    // Create temp directory for logs
    CreateDirectoryA("D:\\temp", NULL);

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
        fmt::print("Check D:\\temp\\hookx_log.txt for hook activity.\n\n");

        // Automatically configure memory address
        fmt::print("=== Automatic Address Configuration ===\n");
        fmt::print("The DLL will automatically read the camera forward address from D:\\temp\\hookx_addresses.txt\n");
        fmt::print("Make sure the address file contains the correct g_cameraForward address.\n\n");

        // Check if address file exists and read current address
        std::ifstream checkFile("D:\\temp\\hookx_addresses.txt");
        bool hasExistingAddress = false;
        uintptr_t existingAddress = 0;

        if (checkFile.is_open()) {
            std::string line;
            while (std::getline(checkFile, line)) {
                if (line.find("g_cameraForward=") == 0) {
                    std::string addressStr = line.substr(16);
                    try {
                        existingAddress = std::stoull(addressStr, nullptr, 16);
                        hasExistingAddress = true;
                        fmt::print("Found existing address in file: 0x{:X}\n", existingAddress);
                    } catch (const std::exception&) {
                        fmt::print("Invalid address format in existing file.\n");
                    }
                    break;
                }
            }
            checkFile.close();
        }

        if (!hasExistingAddress) {
            fmt::print("No valid address file found. Creating default address file.\n");
            fmt::print("Please edit D:\\temp\\hookx_addresses.txt with the correct address.\n");

            // Create a default address file with placeholder
            std::ofstream defaultFile("D:\\temp\\hookx_addresses.txt");
            if (defaultFile.is_open()) {
                defaultFile << "g_cameraForward=0" << std::endl;
                defaultFile.close();
                fmt::print("Created default address file. Please update it with the real address.\n");
            }
        }

        fmt::print("\nThe DLL will automatically monitor the address file for updates.\n");
        fmt::print("Press Enter to exit (this will NOT remove the hook)...\n");
    } else {
        fmt::print("ERROR: Failed to inject DLL into Elden Ring!\n");
        fmt::print("Make sure you're running as administrator.\n");
        fmt::print("Press Enter to exit...\n");
    }

    std::cin.get();
    return 0;
}