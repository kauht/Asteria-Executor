#include "memory.h"
#include "panorama.h"
void* find_hud_element(const char* hud)
{
    using fn = void* (__fastcall*)(const char*);
    static auto addr = M::FindPattern(L"client.dll", "4C 8B DC 53 48 83 EC 50 48 8B 05");

    const auto find_hud = reinterpret_cast<fn>(addr);

    if (find_hud)
        return find_hud(hud);
}




void CCSGO_HudWeaponSelection_ClearHudWeaponIcon(void* hud, int a2, int a3)
{
    using Fn = void(__fastcall*)(void*, int, int);
    static Fn fn = nullptr;

    if (!fn)
    {
        uintptr_t addr = (uintptr_t)M::FindPattern(L"client.dll", "E8 ? ? ? ? 8B F8 C6 84 24 ? ? ? ? ?");
        if (!addr)
            return;

        uintptr_t rel = *reinterpret_cast<int32_t*>(addr + 1);
        uintptr_t fnAddr = addr + 5 + rel;

        fn = reinterpret_cast<Fn>(fnAddr);
    }

    fn(hud, a2, a3);
}
