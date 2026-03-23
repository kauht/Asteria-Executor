#include "skin_changer.h"
#include "game_context.h"
#include "../api/sig_scanner.h"
#include "../lua_manager.h"
#include "../offsets/offsets.hpp"
#include <iostream>

extern LuaManager* g_Lua;

namespace SkinChanger
{
    std::map<int, SkinConfig> weaponSkins;
    KnifeConfig knifeSkin;
    GloveConfig gloveSkin;
    std::mutex configMutex;
    std::atomic<bool> forceUpdate = false;

    #pragma pack(push, 1)
    struct CEconItemAttribute
    {
        uintptr_t vtable;       // 0x00
        uintptr_t owner;        // 0x08
        char pad_0010[32];      // 0x10
        uint16_t defIndex;      // 0x30
        char pad_0032[2];       // 0x32
        float value;            // 0x34
        float initValue;        // 0x38
        int32_t refundableCurrency; // 0x3C
        bool setBonus;          // 0x40
        char pad_0041[7];       // 0x41
    }; // 0x48

    struct CPtrGameVector
    {
        uint64_t size;
        uintptr_t ptr;
    };
    #pragma pack(pop)

    inline uintptr_t regenAddr = 0;
    inline uintptr_t updateSubclassAddr = 0;
    inline bool regenPatched = false;
    inline CEconItemAttribute* g_attrBuffer = nullptr;
    inline uintptr_t lastAppliedWeapon = 0;
    inline int lastAppliedKit = 0;
    inline int lastAppliedKnifeDef = 0;

    const std::map<uint16_t, uint32_t> subclassIdMap = {
        {500, 3933374535}, {503, 3787235507}, {505, 4046390180}, {506, 2047704618},
        {507, 1731408398}, {508, 1638561588}, {509, 2282479884}, {512, 3412259219},
        {514, 2511498851}, {515, 1353709123}, {516, 4269888884}, {517, 1105782941},
        {518, 275962944}, {519, 1338637359}, {520, 3230445913}, {521, 3206681373},
        {522, 2595277776}, {523, 4029975521}, {524, 2463111489}, {525, 365028728},
        {526, 3845286452}
    };

    // From Cs2 Epstein Rage definitions
    constexpr ptrdiff_t m_AttributeManager = 0x1378;
    constexpr ptrdiff_t m_Item = 0x50;
    constexpr ptrdiff_t m_AttributeList = 0x208;
    constexpr ptrdiff_t m_Attributes = 0x8;
    constexpr ptrdiff_t m_iItemIDHigh = 0x1D0;
    constexpr ptrdiff_t m_nFallbackPaintKit = 0x1850;
    constexpr ptrdiff_t m_nFallbackSeed = 0x1854;
    constexpr ptrdiff_t m_flFallbackWear = 0x1858;
    constexpr ptrdiff_t m_nFallbackStatTrak = 0x185C;
    constexpr ptrdiff_t m_pClippingWeapon = 0x3DC0;
    constexpr ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
    constexpr ptrdiff_t m_nSubclassID = 0x388; // CUtlStringToken
    
    // safe 8 byte
    template<typename T>
    inline T SafeRead(uintptr_t addr) {
        if (!addr) return T{};
        __try { return *(T*)addr; }
        __except (EXCEPTION_EXECUTE_HANDLER) { return T{}; }
    }

    template<typename T>
    inline void SafeWrite(uintptr_t addr, T value) {
        if (!addr) return;
        __try { *(T*)addr = value; }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    inline CEconItemAttribute MakeAttribute(uint16_t def, float value)
    {
        CEconItemAttribute attr{};
        attr.defIndex = def;
        attr.value = value;
        attr.initValue = value;
        return attr;
    }

    void Init()
    {
        if (regenAddr == 0) {
            regenAddr = (uintptr_t)ScanPattern("client.dll", "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10");
            if (regenAddr)
            {
                uint16_t combinedOffset = static_cast<uint16_t>(m_AttributeManager + m_Item + m_AttributeList + m_Attributes);
                DWORD oldProtect;
                if (VirtualProtect(reinterpret_cast<void*>(regenAddr + 0x52), 2, PAGE_EXECUTE_READWRITE, &oldProtect))
                {
                    *reinterpret_cast<uint16_t*>(regenAddr + 0x52) = combinedOffset;
                    VirtualProtect(reinterpret_cast<void*>(regenAddr + 0x52), 2, oldProtect, &oldProtect);
                    regenPatched = true;
                }
            }
        }
        
        if (updateSubclassAddr == 0) {
            updateSubclassAddr = (uintptr_t)ScanPattern("client.dll", "40 53 48 83 EC 30 48 8B D9 E8 ? ? ? ? 48 8B 03");
        }
    }

    void CallRegen()
    {
        if (!regenAddr || !regenPatched) return;
        __try {
            typedef void(__fastcall* RegenFn)();
            auto fn = reinterpret_cast<RegenFn>(regenAddr);
            fn();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    void CallUpdateSubclass(uintptr_t weapon)
    {
        if (!updateSubclassAddr || !weapon) return;
        __try {
            typedef void(__fastcall* UpdateSubclassFn)(void*);
            auto fn = reinterpret_cast<UpdateSubclassFn>(updateSubclassAddr);
            fn(reinterpret_cast<void*>(weapon));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { }
    }

    void CreateAttributes(uintptr_t item, int paintKit, int seed, float wear)
    {
        uintptr_t attrListAddr = item + m_AttributeManager + m_Item + m_AttributeList + m_Attributes;
        CPtrGameVector existing = SafeRead<CPtrGameVector>(attrListAddr);
        if (existing.size > 0 || existing.ptr != 0) return;

        if (!g_attrBuffer)
        {
            g_attrBuffer = reinterpret_cast<CEconItemAttribute*>(VirtualAlloc(nullptr, sizeof(CEconItemAttribute) * 3, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!g_attrBuffer) return;
        }

        g_attrBuffer[0] = MakeAttribute(6, static_cast<float>(paintKit));
        g_attrBuffer[1] = MakeAttribute(7, static_cast<float>(seed));
        g_attrBuffer[2] = MakeAttribute(8, wear);

        CPtrGameVector newList;
        newList.size = 3;
        newList.ptr = reinterpret_cast<uintptr_t>(g_attrBuffer);
        SafeWrite<CPtrGameVector>(attrListAddr, newList);
    }

    void RemoveAttributes(uintptr_t item)
    {
        uintptr_t attrListAddr = item + m_AttributeManager + m_Item + m_AttributeList + m_Attributes;
        CPtrGameVector empty{};
        SafeWrite<CPtrGameVector>(attrListAddr, empty);
    }

    void ApplyAndRegen(uintptr_t weapon, const SkinConfig& skin, uint16_t defIndex)
    {
        uintptr_t item = weapon + m_AttributeManager + m_Item;
        uint32_t origItemIDHigh = SafeRead<uint32_t>(item + m_iItemIDHigh);

        SafeWrite<uint32_t>(item + m_iItemIDHigh, 0xFFFFFFFF);
        SafeWrite<int32_t>(weapon + m_nFallbackPaintKit, skin.paintKit);
        SafeWrite<float>(weapon + m_flFallbackWear, skin.wear);
        SafeWrite<int32_t>(weapon + m_nFallbackSeed, skin.seed);
        SafeWrite<int32_t>(weapon + m_nFallbackStatTrak, skin.statTrak);

        CreateAttributes(weapon, skin.paintKit, skin.seed, skin.wear);
        CallRegen();

        RemoveAttributes(weapon);
        SafeWrite<uint32_t>(item + m_iItemIDHigh, origItemIDHigh);
        SafeWrite<int32_t>(weapon + m_nFallbackPaintKit, 0);
        SafeWrite<float>(weapon + m_flFallbackWear, 0.0f);
        SafeWrite<int32_t>(weapon + m_nFallbackSeed, 0);
        SafeWrite<int32_t>(weapon + m_nFallbackStatTrak, -1);
    }

    void TickInner()
    {
        Init();

        // Getting pawn explicitly handled since standard dwLocalPlayerPawn isn't saved directly in GameContext. 
        uintptr_t dwLocalPlayerPawn = cs2_dumper::offsets::client_dll::dwLocalPlayerPawn; 
        uintptr_t localPawn = SafeRead<uintptr_t>(GameContext::Get().client_base + dwLocalPlayerPawn);
        if (!localPawn) return;

        uint8_t lifeState = SafeRead<uint8_t>(localPawn + 0x35C);
        int32_t health = SafeRead<int32_t>(localPawn + 0x354);
        if (lifeState != 0 || health <= 0)
        {
            lastAppliedWeapon = 0;
            return;
        }

        bool force = forceUpdate.load();
        std::lock_guard<std::mutex> lock(configMutex);

        uintptr_t activeWeapon = SafeRead<uintptr_t>(localPawn + m_pClippingWeapon);
        if (activeWeapon)
        {
            uintptr_t item = activeWeapon + m_AttributeManager + m_Item;
            uint16_t defIndex = SafeRead<uint16_t>(item + m_iItemDefinitionIndex);

            bool isWeapon = (defIndex > 0 && defIndex < 70) || (defIndex >= 500 && defIndex < 600);
            if (isWeapon && defIndex != 31)
            {
                if (defIndex >= 500 && knifeSkin.enabled && knifeSkin.paintKit > 0)
                {
                    // Knife flow
                    bool needsApply = force || (activeWeapon != lastAppliedWeapon) || (knifeSkin.paintKit != lastAppliedKit) || (knifeSkin.defIndex != lastAppliedKnifeDef);
                    if (needsApply)
                    {
                        SkinConfig tempSkin;
                        tempSkin.paintKit = knifeSkin.paintKit;
                        tempSkin.wear = knifeSkin.wear;
                        tempSkin.seed = knifeSkin.seed;
                        tempSkin.statTrak = -1;

                        SafeWrite<uint32_t>(item + m_iItemIDHigh, 0);

                        // Overwrite SubclassID if definition changed
                        if (knifeSkin.defIndex != 0) {
                            SafeWrite<uint16_t>(item + m_iItemDefinitionIndex, knifeSkin.defIndex);
                            auto hashIt = subclassIdMap.find(knifeSkin.defIndex);
                            if (hashIt != subclassIdMap.end()) {
                                SafeWrite<uint32_t>(activeWeapon + m_nSubclassID, hashIt->second);
                                CallUpdateSubclass(activeWeapon);
                            }
                        }

                        ApplyAndRegen(activeWeapon, tempSkin, knifeSkin.defIndex);
                        lastAppliedWeapon = activeWeapon;
                        lastAppliedKit = knifeSkin.paintKit;
                        lastAppliedKnifeDef = knifeSkin.defIndex;
                    }
                }
                else
                {
                    // Weapon flow
                    int lookupIndex = defIndex;
                    if (defIndex == 42 || defIndex == 59)
                    {
                        for (auto& [key, cfg] : weaponSkins)
                            if (key >= 500 && key < 600 && cfg.enabled) { lookupIndex = key; break; }
                    }

                    auto it = weaponSkins.find(lookupIndex);
                    if (it != weaponSkins.end() && it->second.enabled && it->second.paintKit > 0)
                    {
                        const SkinConfig& skin = it->second;
                        bool needsApply = force || (activeWeapon != lastAppliedWeapon) || (skin.paintKit != lastAppliedKit);

                        if (needsApply)
                        {
                            SafeWrite<uint32_t>(item + m_iItemIDHigh, 0);
                            ApplyAndRegen(activeWeapon, skin, defIndex);
                            lastAppliedWeapon = activeWeapon;
                            lastAppliedKit = skin.paintKit;
                        }
                    }
                }
            }
        }

        if (force) forceUpdate.store(false);
    }

    void Tick()
    {
        __try {
            TickInner();
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            lastAppliedWeapon = 0;
        }
    }
}
