#pragma once
#include <windows.h>
#include <string>

class Injector {
public:
    static DWORD GetProcessIdByName(const std::string& processName);
    static bool InjectDll(DWORD pid, const std::string& dllPath);

private:
    static BOOL UnhookMethod(HANDLE hProcess, const char* methodName, const wchar_t* dllName);
    static bool ApplyHookBypass(HANDLE hProcess);
};
