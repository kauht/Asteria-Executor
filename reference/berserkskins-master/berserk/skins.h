#pragma once  
#include <vector>  
#include <string>  
#include <array> // Добавлено для исправления ошибки  
#include "CCPlayerInventory.h"
#include "entity.h"
#include "CEconItem.h"

class CCSPlayerInventory;
namespace S
{
    class c_dumped_skins {
    public:
        const char* m_name = "";
        int m_id = 0;
        int m_rarity = 0;
    };

    class c_dumped_item {
    public:
        const char* m_name = "";
        uint16_t m_def_index = 0;
        int m_rarity = 0;
        bool m_unusual_item = false;
        std::vector<c_dumped_skins> m_dumped_skins{};
        c_dumped_skins* m_selected_skin = nullptr;
    };

    extern std::vector<c_dumped_item> m_dumped_items;

    struct AgentData {
        std::string model;
        std::string name;
    };

    extern std::vector<AgentData> agents;

    struct KnivesData {
        std::array<int, 20> m_knife_idxs = { 500, 503, 505, 506, 507, 508, 509, 512, 514, 515, 516, 517, 518, 519, 520, 521, 522, 523, 525, 526 };
        std::string model;
        std::string name;
    };

    extern std::vector<KnivesData> knives;

    void set_agent(int stage); // Функция для установки агента  
    void ApplyKnifeSkins(C_CSPlayerPawn* pLocalPawn, CCSPlayerInventory* pInventory, C_CS2HudModelWeapon* pHudModelWeapon);
    void AddEconItemToList(CEconItem* pItem, float paintKit, float paintSeed, float paintWear, bool legacy);
    void ApplyWeaponSkins(C_CSPlayerPawn* pLocalPawn, CCSPlayerInventory* pInventory, C_CS2HudModelWeapon* pHudModelWeapon);
    void ApplyGloves();
    void onEquipItemInLoadout(void* thisptr, int iTeam, int iSlot, uint64_t iItemID);
	void Shutdown();
    void clear_hud(C_EconItemView* pWeaponItemView);
} // namespace skins
