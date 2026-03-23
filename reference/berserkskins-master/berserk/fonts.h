#pragma once
#include "imgui/imgui.h"

namespace FONTS {
    extern ImFont* Regular;
    extern ImFont* Bold;
    extern ImFont* Icons;
    extern ImFont* GunIcon;

    void Initialize(ImGuiIO& io);
    const char* WeaponToFontLetter(const char* weapon);
    inline ImFont* GetRegular() { return Regular; }
    inline ImFont* GetIcons() { return Icons; }
    inline ImFont* GetGunIcons() { return GunIcon; }
    inline ImFont* GetBold() { return Bold; }
    bool IsInitialized();
}
