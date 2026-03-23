#pragma once
void* find_hud_element(const char* value);

#include "memory.h"
#include <cstdint>

using tFindHudElement = void* (__fastcall*)(const char* name);

namespace CCSGOHudElement
{
    template<typename T>
    inline T* Find(const char* szHudName)
    {
        using tFindHudElement = void* (__fastcall*)(const char* name);
        static tFindHudElement fn = nullptr;

        if (!fn)
        {
            uintptr_t addr = (uintptr_t)M::FindPattern(L"client.dll", "4C 8B DC 53 48 83 EC 50 48 8B 05");
            if (!addr)
                return nullptr;

            fn = reinterpret_cast<tFindHudElement>(addr);
        }

        return reinterpret_cast<T*>(fn(szHudName));
    }
}

void CCSGO_HudWeaponSelection_ClearHudWeaponIcon(void* hud, int a2, int a3);