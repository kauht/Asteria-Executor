#include "injector.h"
#include <tlhelp32.h>
#include <iostream>

DWORD Injector::GetProcessIdByName(const std::string& processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32First(snapshot, &processEntry)) {
            do {
                if (processName == processEntry.szExeFile) {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

bool Injector::InjectDll(DWORD pid, const std::string& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return false;

    // Apply Hook Bypass from AnarchyInjector
    ApplyHookBypass(hProcess);

    void* pRemoteBuf = VirtualAllocEx(hProcess, NULL, dllPath.length() + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteBuf) {
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath.c_str(), dllPath.length() + 1, NULL)) {
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    PTHREAD_START_ROUTINE pLoadLibrary = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!pLoadLibrary) {
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, pRemoteBuf, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return true;
}

BOOL Injector::UnhookMethod(HANDLE hProcess, const char* methodName, const wchar_t* dllName) {
    HMODULE hModule = GetModuleHandleW(dllName);
    if (!hModule) return FALSE;

    LPVOID oriMethodAddr = (LPVOID)GetProcAddress(hModule, methodName);
    if (!oriMethodAddr) return FALSE;

    unsigned char originalBytes[6];
    memcpy(originalBytes, oriMethodAddr, 6);

    // Write original bytes into target memory to restore hook
    if (!WriteProcessMemory(hProcess, oriMethodAddr, originalBytes, 6, NULL)) {
        return FALSE;
    }
    return TRUE;
}

bool Injector::ApplyHookBypass(HANDLE hProcess) {
    BOOL res = TRUE;
    res &= UnhookMethod(hProcess, "LoadLibraryExW", L"kernel32");
    res &= UnhookMethod(hProcess, "VirtualAlloc", L"kernel32");
    res &= UnhookMethod(hProcess, "FreeLibrary", L"kernel32");
    res &= UnhookMethod(hProcess, "LoadLibraryExA", L"kernel32");
    res &= UnhookMethod(hProcess, "LoadLibraryW", L"kernel32");
    res &= UnhookMethod(hProcess, "LoadLibraryA", L"kernel32");
    res &= UnhookMethod(hProcess, "VirtualAllocEx", L"kernel32");
    res &= UnhookMethod(hProcess, "LdrLoadDll", L"ntdll");
    res &= UnhookMethod(hProcess, "NtOpenFile", L"ntdll");
    res &= UnhookMethod(hProcess, "VirtualProtect", L"kernel32");
    res &= UnhookMethod(hProcess, "VirtualProtectEx", L"kernel32");
    return res != FALSE;
}
