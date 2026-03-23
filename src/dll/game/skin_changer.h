#pragma once
#include <Windows.h>
#include <cstdint>
#include <map>
#include <mutex>
#include <atomic>

namespace SkinChanger
{
    struct SkinConfig
    {
        int paintKit = 0;
        float wear = 0.001f;
        int seed = 0;
        int statTrak = -1;
        bool enabled = false;
    };

    struct KnifeConfig
    {
        int defIndex = 0;
        int paintKit = 0;
        float wear = 0.001f;
        int seed = 0;
        bool enabled = false;
    };

    struct GloveConfig
    {
        int defIndex = 0;
        int paintKit = 0;
        float wear = 0.001f;
        int seed = 0;
        bool enabled = false;
    };

    extern std::map<int, SkinConfig> weaponSkins;
    extern KnifeConfig knifeSkin;
    extern GloveConfig gloveSkin;
    extern std::mutex configMutex;
    extern std::atomic<bool> forceUpdate;

    // Call this inside hook manager once per frame / tick context
    void Tick();

    // Setup helper
    void Init();
}
