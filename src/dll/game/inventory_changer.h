#pragma once
#include <cstdint>
#include <vector>
#include <string>

// Formatted CEconItem structures identified via reversing
struct CEconItem {
    // VTable usually at 0x0
    void* m_pVTable; 
    
    // We only need specific offsets mapped back to local variables
    // In source, it supports dynamic attributes
    // Offset targets found commonly:
    // uint64_t m_ulID;
    // uint16_t m_unDefIndex;
    // uint8_t m_unQuality;
    // uint16_t m_nSubclassID;
};

class InventoryChanger {
public:
    static bool Initialize();
    static void Tick();
    static CEconItem* CreateItem();
    static bool AddItemToInventory(CEconItem* pItem);
    static int GetInventoryCount();
    static int GetEconItemRawWord(int index, int wordIndex);
    static int GetEconItemCacheCount();
    static int GetLastErrorCode();
    static int GetRawWord(int index);
    static void ScanInventoryManager();
    static int GetScanResultCount();
    static std::string GetScanResult(int index);
    
    // Config items to bind to Lua
    struct ItemConfig {
        uint16_t defIndex;
        uint32_t paintKit;
        float wear;
        uint32_t seed;
    };
};
