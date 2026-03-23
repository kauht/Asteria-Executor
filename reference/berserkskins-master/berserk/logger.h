#pragma once
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
namespace logger
{
    inline void initialize()
    {
        if (AllocConsole()) {
            SetConsoleTitleA("Custom Console");

            FILE* pCout;
            FILE* pCerr;
            FILE* pCin;

            freopen_s(&pCout, "CONOUT$", "w", stdout);
            freopen_s(&pCerr, "CONOUT$", "w", stderr);
            freopen_s(&pCin, "CONIN$", "r", stdin);
        }
    }

    inline void shutdown()
    {
        FILE* pCout;
        FILE* pCerr;
        FILE* pCin;

        freopen_s(&pCout, "nul", "w", stdout);
        freopen_s(&pCerr, "nul", "w", stderr);
        freopen_s(&pCin, "nul", "r", stdin);

        FreeConsole();
    }
}