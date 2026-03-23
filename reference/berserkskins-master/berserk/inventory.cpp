#include "inventory.h"
#include "interfaces.h"
#include <algorithm>
static std::vector<DumpedItem_t> vecDumpedItems;
static DumpedItem_t* pSelectedItem = nullptr;

static bool is_paint_kit_for_weapon(C_PaintKit* paint_kit, C_EconItemDefinition* weapon)
{

    auto name = weapon->get_simple();

    std::string path = "panorama/images/econ/default_generated/";
    path += name;
    path += "_";
    path += paint_kit->m_name;
    path += "_light_png.vtex_c";
    if (I::file_system->exists(path.c_str(), "GAME")) {
        return true;
    }

    return false;

}


bool skinsDump() {
    vecDumpedItems.clear();
    auto item_schema = I::iclient->get_econ_item_system()->get_econ_item_schema();
    if (!item_schema)
    {
        return false;
    }
    int i = 0;
    if (vecDumpedItems.empty())
    {
        const auto& vecItems = item_schema->GetSortedItemDefinitionMap();

        const auto& vecPaintKits = item_schema->GetPaintKits();

        const auto& vecStickers = item_schema->GetStickers();

        std::vector<void*> vecItemss;
        for (auto it = vecItems.begin(); it != vecItems.end(); ++it) {
            vecItemss.push_back(it->m_value); 
        }

        for (const auto& it : vecItems)
        {
            C_EconItemDefinition* pItem = it.m_value;
            if (!pItem)
                continue;
            const bool isWeapon = pItem->is_weapon();
            const bool isKnife = pItem->is_knife(true);
            const bool isGloves = pItem->is_glove(true);
            const bool IsAgent = pItem->is_agent();
            const bool isOther = !isKnife && !isWeapon && !IsAgent && !isGloves;

            if (!isOther && !isKnife && !isWeapon && !IsAgent && !isGloves)
                continue;
            const char* itemBaseName = pItem->m_pszItemBaseName;

            if (!itemBaseName || itemBaseName[0] == '\0')
                continue;
            const uint16_t defIdx = pItem->m_nDefIndex;

            DumpedItem_t dumpedItem;
            dumpedItem.m_name = I::localize->find_key(pItem->m_pszItemBaseName);

            if (dumpedItem.m_name[0] == '#' || dumpedItem.m_name == "")
                continue;
            dumpedItem.m_defIdx = defIdx;
            dumpedItem.m_rarity = pItem->GetRarity(); // R0 for all non-skin items
            if (isWeapon || isKnife || isGloves || IsAgent || isOther)
            {
                const char* simple = pItem->get_simple();
                if (simple && simple[0] != '\0')
                    dumpedItem.m_simpleName = simple; // e.g. weapon_ak47
            }
            dumpedItem.isWeapon = isWeapon;
            dumpedItem.isKnife = isKnife;
            dumpedItem.isGloves = isGloves;
            dumpedItem.isAgent = IsAgent;
            dumpedItem.isOther = isOther;

            if (isKnife || isGloves || isWeapon || IsAgent || isOther)
            {
                dumpedItem.m_unusualItem = true;
            }

            if (IsAgent) {
                dumpedItem.m_dumpedSkins.emplace_back("Vanilla", 0, pItem->GetRarity()); // R0 for agents
            }
            else if (isOther) {
                dumpedItem.m_dumpedSkins.emplace_back("Vanilla", 0, pItem->GetRarity()); // R0 for other items
            }
            i++;

          
            if (!IsAgent && !isOther)
            {
                for (const auto& it : vecPaintKits)
                {
                    C_PaintKit* pPaintKit = it.m_value;
                    if (!pPaintKit || pPaintKit->m_id == 0 || pPaintKit->m_id == 9001)
                        continue;

                    if (!is_paint_kit_for_weapon(pPaintKit, pItem))
                        continue;

                    DumpedSkin_t dumpedSkin;
                    dumpedSkin.m_name = I::localize->find_key(pPaintKit->m_description_tag);
                    if (dumpedSkin.m_name[0] == '#' || dumpedSkin.m_name == "")
                        continue;
                    dumpedSkin.m_ID = pPaintKit->m_id;
                    dumpedSkin.legacy = pPaintKit->legacy();
                    // token for icon path (e.g., cu_awp_asimov)
                    if (pPaintKit->m_name)
                        dumpedSkin.m_tokenName = pPaintKit->m_name;

                    dumpedSkin.m_rarity = std::clamp<int>(
                        static_cast<int>(pItem->GetRarity() + pPaintKit->m_rarity - 1),
                        0,
                        (pPaintKit->m_rarity == 7) ? 7 : 6
                    );
                    dumpedItem.m_dumpedSkins.emplace_back(dumpedSkin);

                }
            }

            if (!dumpedItem.m_dumpedSkins.empty() && isWeapon) {
                std::sort(dumpedItem.m_dumpedSkins.begin(),
                    dumpedItem.m_dumpedSkins.end(),
                    [](const DumpedSkin_t& a, const DumpedSkin_t& b) {
                        return a.m_rarity > b.m_rarity;
                    });
            }

            vecDumpedItems.emplace_back(dumpedItem);

        }

        if (vecDumpedItems.empty()) {
            printf("[-] No items found to dump.\n");
            return false;
        }
		printf("[+] Dumped %zu items.\n", vecDumpedItems.size());
        return true;

    }

}
namespace INVENTORY {
    static bool dumped = false;
    bool Dump() {

        if (!dumped && skinsDump()) {
            dumped = true;
            return true;
        }
        return false;
    }

    std::vector<DumpedItem_t>& GetDumpedItems()
    {
        return vecDumpedItems;
    }
    bool IsDumped()
    {
        return dumped;
    }

}