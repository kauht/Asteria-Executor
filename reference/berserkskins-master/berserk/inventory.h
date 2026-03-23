#pragma once
#include <string>
#include <vector>

class CImageProxySource;

struct DumpedSkin_t
{
    std::string m_name = "";
    int m_ID = 0;
    int m_rarity = 0;
    bool legacy = false;
    std::string m_tokenName = "";
    CImageProxySource* m_image = nullptr;
};
struct DumpedItem_t
{
    std::string m_name = "";
    uint16_t m_defIdx = 0;
    int m_rarity = 0;
    CImageProxySource* m_image = nullptr;
    std::string m_simpleName = ""; // simple weapon name (weapon_ak47, etc.)
    int m_previewSkinIndex = -1;
    bool m_unusualItem = false;
    std::vector<DumpedSkin_t> m_dumpedSkins{};
    DumpedSkin_t* pSelectedSkin = nullptr;
    bool isWeapon = false;
    bool isKnife = false;
    bool isGloves = false;
    bool isAgent = false;
    bool isOther = false;
};
namespace INVENTORY {
    bool Dump();
    std::vector<DumpedItem_t>& GetDumpedItems();
    bool IsDumped();
}