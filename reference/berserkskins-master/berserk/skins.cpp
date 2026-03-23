#include "skins.h"
#include "interfaces.h"
#include <algorithm>
#include "enums.h"
#include <map>
#include "panorama.h"
#include "inventory_p.h"
#include "i_event_manager.h"
#include <thread>
#include <functional>
//static bool is_paint_kit_for_weapon(c_paint_kit* paint_kit, const char* weapon_id)
//{
//    auto path = "panorama/images/econ/default_generated/" + std::string(weapon_id) + "_" + paint_kit->PaintKitName() + "_light_png.vtex_c";
//    return I::m_file_system->exists(path.c_str(), "GAME");
//}
struct AddedItemInfo
{
    uint64_t id;
    float paintKit;
    float paintSeed;
    float paintWear;
    bool legacy;
};
inline std::vector<AddedItemInfo> g_vecAddedItemsIDs;
struct CCSGO_HudWeaponSelection
{
    // ничего внутри описывать не нужно
};
struct hud_weapons_t {
    // Число оружий находится по оффсету 0x58


    // Можно добавить другие поля/методы при необходимости
};

namespace S
{
    std::vector<AgentData> agents;
    std::vector<KnivesData> knives;
    std::vector<c_dumped_item> m_dumped_items;

    void clear_hud(C_EconItemView* pWeaponItemView) {
        printf("clear\n");
        auto* pCCSGO_HudWeaponSelection = CCSGOHudElement::Find<CCSGO_HudWeaponSelection>("HudWeaponSelection");

        if (pCCSGO_HudWeaponSelection)
        {
            pWeaponItemView->pCEconItemDescription(); // C_EconItemView + 0x200 = 0 

            CCSGO_HudWeaponSelection_ClearHudWeaponIcon(
                reinterpret_cast<CCSGO_HudWeaponSelection*>(
                    (uintptr_t)pCCSGO_HudWeaponSelection - 0x98), 0 , 0);
        }

    }
    void set_name( C_EconItemView* item) {

        const char* name_tag = "neggr1337";
        memcpy(&item->m_szCustomName(), name_tag, sizeof(name_tag));
    }
    void ApplyGloves()
    {
        if (!I::engine || !I::engine->is_connected() || !I::engine->is_in_game()) return;

        static uint8_t updateFrames = 0;

        CCSPlayerInventory* inventory = CCSPlayerInventory::GetInstance();
        if (!inventory)
            return;
        static float flLastSpawnTimeIndex = 0.f;
        auto localcontroller = CCSPlayerController::GetLocalPlayerController();
        if (!localcontroller)
            return;
        if (localcontroller->IsPawnAlive() == false)
            return;
        auto local_player = C_CSPlayerPawn::GetLocalPawn();
        if (!local_player)
            return;
        C_EconItemView* pGloves = &local_player->m_EconGloves();

        C_EconItemView* pTargetGlovesItemView = inventory->GetItemInLoadout(local_player->team(), 41);

        if (pGloves && pTargetGlovesItemView) {
            const auto iGlovesItemID = pGloves->m_iItemID();
            const auto iTargetGlovesItemID = pTargetGlovesItemView->m_iItemID();
            if (iGlovesItemID != iTargetGlovesItemID || local_player->m_flLastSpawnTimeIndex() != flLastSpawnTimeIndex) {
                updateFrames = 3;
                pGloves->m_iItemDefinitionIndex() = pTargetGlovesItemView->m_iItemDefinitionIndex();

                pGloves->m_iItemID() = pTargetGlovesItemView->m_iItemID();
                pGloves->m_iItemIDHigh() = pTargetGlovesItemView->m_iItemIDHigh();
                pGloves->m_iItemIDLow() = pTargetGlovesItemView->m_iItemIDLow();
                pGloves->m_iAccountID() = pTargetGlovesItemView->m_iAccountID();


                flLastSpawnTimeIndex = local_player->m_flLastSpawnTimeIndex();
            }
            if (updateFrames) {
                pGloves->m_bInitialized() = true;
                local_player->SetBody_Group();
                local_player->m_bNeedToReApplyGloves() = true;

                updateFrames--;
            }
        }
        //printf("%f %d %d\n", flLastSpawnTimeIndex, pGloves->m_iItemID(), pTargetGlovesItemView->m_iItemID());
    }
    void set_agent(int stage)
    {
        if (!I::engine || !I::engine->is_connected() || !I::engine->is_in_game()) return;
        auto localcontroller = CCSPlayerController::GetLocalPlayerController();
        if (!localcontroller)
            return;
        if (localcontroller->IsPawnAlive() == false)
            return;
        auto local_player = C_CSPlayerPawn::GetLocalPawn();
        if (!local_player || stage != 6)
            return;
        static float flLastSpawnTimeIndex = 666.f;
        if (flLastSpawnTimeIndex == local_player->m_flLastSpawnTimeIndex()) return;

        CCSPlayerInventory* inventory = CCSPlayerInventory::GetInstance();
        if (!inventory)
            return;

        C_EconItemView* item_view_ = inventory->GetItemInLoadout(local_player->team(), 38);
        auto item_view_loadout = item_view_->get_static_data();

        if (!item_view_loadout)
            return;

        local_player->set_model(item_view_loadout->get_model_name());
        flLastSpawnTimeIndex = local_player->m_flLastSpawnTimeIndex();


    }
    void ApplyWeaponSkins(C_CSPlayerPawn* pLocalPawn, CCSPlayerInventory* pInventory, C_CS2HudModelWeapon* pHudModelWeapon) {

        if (!I::engine || !I::engine->is_connected() || !I::engine->is_in_game()) return;
        auto localcontroller = CCSPlayerController::GetLocalPlayerController();
        if (!localcontroller)
            return;
        if (localcontroller->IsPawnAlive() == false)
            return;
        auto pWeaponServices = pLocalPawn->GetWeaponServices();

        if (pWeaponServices == nullptr)
            return;


        C_CSWeaponBase* pWeapon = pLocalPawn->GetActiveWeaponPlayer();
        if (!pWeapon) {
            return;
        }


        if (pWeapon == nullptr)
            return;


        const uint64_t steamID = pInventory->GetOwner().m_id;
        if (pWeapon->GetOriginalOwnerXuid() != steamID)
            return;

        CGameSceneNode* pWeaponSceneNode = pWeapon->GetGameSceneNode();
        if (!pWeaponSceneNode)
            return;
        C_AttributeContainer* pAttributeContainer = &pWeapon->m_AttributeManager();
        if (!pAttributeContainer)
            return;
        C_EconItemView* pWeaponItemView = &pAttributeContainer->m_Item();
        if (!pWeaponItemView)
            return;
        C_EconItemDefinition* pWeaponDefinition = pWeaponItemView->get_static_data();
        if (!pWeaponDefinition)
            return;
        if (pWeaponDefinition->is_knife(false) == true)
            return;

        const auto nLoadoutSlot = pWeaponDefinition->GetLoadoutSlot();


        C_EconItemView* pWeaponInLoadoutItemView = pInventory->GetItemInLoadout(pWeapon->m_iOriginalTeamNumber(), nLoadoutSlot);
        if (!pWeaponInLoadoutItemView) {
            return;
        }

        C_EconItemDefinition* pWeaponInLoadoutDefinition = pWeaponInLoadoutItemView->get_static_data();
        if (!pWeaponInLoadoutDefinition) {
            return;
        }

        if (pWeaponInLoadoutDefinition->m_nDefIndex != pWeaponDefinition->m_nDefIndex) {
            return;
        }

        auto it = std::find_if(
            g_vecAddedItemsIDs.begin(),
            g_vecAddedItemsIDs.end(),
            [&](const AddedItemInfo& info) {
                return info.id == pWeaponInLoadoutItemView->m_iItemID();
            }
        );


        // if (pWeaponItemView->m_iAccountID() != uint32_t(pInventory->GetOwner().m_id)) return;
        pWeaponItemView->m_iItemID() = pWeaponInLoadoutItemView->m_iItemID();
        pWeaponItemView->m_iItemIDHigh() = pWeaponInLoadoutItemView->m_iItemIDHigh();
        pWeaponItemView->m_iItemIDLow() = pWeaponInLoadoutItemView->m_iItemIDLow();
        pWeaponItemView->m_iAccountID() = uint32_t(pInventory->GetOwner().m_id);

        pWeaponItemView->m_bDisallowSOCm() = false;

        int bLegacy = 0;
        if (it != g_vecAddedItemsIDs.end())
        {
            bLegacy = it->legacy ? 1 : 0;

        }
        //pWeapon->AddStattrakEntity();
        //pWeapon->AddNametagEntity();
        pWeaponSceneNode->set_mesh_group_mask(1 + bLegacy);

        if (pHudModelWeapon) {
            CGameSceneNode* pHudModelWeaponSceneNode = pHudModelWeapon->GetGameSceneNode();
            if (pHudModelWeaponSceneNode) {
                pHudModelWeaponSceneNode->set_mesh_group_mask(1 + bLegacy);
                //for (int mask = 0; mask <= 5; ++mask) {
                //    pHudModelWeaponSceneNode->set_mesh_group_mask(mask + bLegacy);
                //}
            }
        }
    }
    bool isKnifeModelTOrCT(const char* knifeModel) {
        if (!knifeModel)
            return false;

        size_t len = std::strlen(knifeModel);

        if (std::strstr(knifeModel, "default_ct_ag2.vmdl"))
            return true;

        if (std::strstr(knifeModel, "default_t_ag2.vmdl"))
            return true;

        return false;
    }
    void call_later(int ms, std::function<void()> func)
    {
        std::thread([ms, func]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            func();
            }).detach();
    }
    void ApplyKnifeSkins(C_CSPlayerPawn* pLocalPawn, CCSPlayerInventory* pInventory, C_CS2HudModelWeapon* pHudModelWeapon) {
        if (!pHudModelWeapon) {
            return;
        }
        auto localcontroller = CCSPlayerController::GetLocalPlayerController();
        if (!localcontroller)
            return;
        if (localcontroller->IsPawnAlive() == false)
            return;


        auto pWeaponServices = pLocalPawn->GetWeaponServices();
        if (!pWeaponServices) {
            return;
        }
        C_CSWeaponBase* pWeapon = pLocalPawn->GetActiveWeaponPlayer();
        if (!pWeapon) {

            return;
        }


        C_AttributeContainer* pAttributeContainer = &pWeapon->m_AttributeManager();
        if (!pAttributeContainer) {
            return;
        }

        C_EconItemView* pWeaponItemView = &pAttributeContainer->m_Item();
        if (!pWeaponItemView) {
            return;
        }

        C_EconItemDefinition* pWeaponDefinition = pWeaponItemView->get_static_data();
        if (!pWeaponDefinition) {
            return;
        }

        if (!pWeaponDefinition->is_knife(false)) {
            return;
        }
        auto pWeaponVData = pWeapon->GetWeaponVData();
        if (!pWeaponVData) {
            return;
        }
        const auto nLoadoutSlot = pWeaponDefinition->GetLoadoutSlot();

        C_EconItemView* pWeaponInLoadoutItemView = pInventory->GetItemInLoadout(pWeapon->m_iOriginalTeamNumber(), nLoadoutSlot);
        if (!pWeaponInLoadoutItemView) {
            return;
        }

        C_EconItemDefinition* pWeaponInLoadoutDefinition = pWeaponInLoadoutItemView->get_static_data();
        if (!pWeaponInLoadoutDefinition) {
            return;
        }
        static float flLastSpawnTimeIndex = 0.f;
        auto pawn = C_CSPlayerPawn::GetLocalPawn();

        pWeaponItemView->m_iItemID() = pWeaponInLoadoutItemView->m_iItemID();
        pWeaponItemView->m_iItemIDHigh() = pWeaponInLoadoutItemView->m_iItemIDHigh();
        pWeaponItemView->m_iItemIDLow() = pWeaponInLoadoutItemView->m_iItemIDLow();
        pWeaponItemView->m_iAccountID() = uint32_t(pInventory->GetOwner().m_id);

        pWeaponItemView->m_bDisallowSOCm() = false;
        pWeaponItemView->m_bRestoreCustomMaterialAfterPrecache() = true;
        static const std::map<uint16_t, uint64_t> m_subclassIdMap = { {500, 3933374535}, {503, 3787235507}, {505, 4046390180}, {506, 2047704618}, {507, 1731408398}, {508, 1638561588}, {509, 2282479884}, {512, 3412259219}, {514, 2511498851}, {515, 1353709123}, {516, 4269888884}, {517, 1105782941}, {518, 275962944}, {519, 1338637359}, {520, 3230445913}, {521, 3206681373}, {522, 2595277776}, {523, 4029975521}, {524, 2463111489}, {525, 365028728}, {526, 3845286452}, };

        if (pWeaponDefinition->is_knife(false)) {
            static int nLastUpdatedWeaponEntry = 0;

            const char* knifeModel = pWeaponInLoadoutDefinition->get_model_name();
            if (isKnifeModelTOrCT(knifeModel)) {
                return; // просто выходим из функции
            }
            //const auto targetSubclassID = m_subclassIdMap.at(pWeaponInLoadoutItemView->m_iItemDefinitionIndex());
            if (pHudModelWeapon) {

                if (nLastUpdatedWeaponEntry != pHudModelWeapon->m_hOwnerEntity().GetEntryIndex()) {
                    const auto targetSubclassID = m_subclassIdMap.at(pWeaponInLoadoutItemView->m_iItemDefinitionIndex());

                    pWeaponItemView->m_iItemDefinitionIndex() = pWeaponInLoadoutItemView->m_iItemDefinitionIndex();

                    pWeapon->m_nSubclassID() = targetSubclassID;

                    pWeapon->UpdateSubClass();
                    nLastUpdatedWeaponEntry = pHudModelWeapon->m_hOwnerEntity().GetEntryIndex();
                    //auto vdata = pWeapon->GetWeaponVData();
                    //if (!vdata)
                    //    return;
                    //vdata->szName() = pWeaponInLoadoutDefinition->get_simple();
                    //pWeapon->UpdateWeaponData();
                   //pWeapon->UpdateVData();
                    call_later(500, [pWeaponItemView]() {
                        S::clear_hud(pWeaponItemView);
                        });

                    

                }




                pWeapon->set_model(knifeModel);
                //pHudModelWeapon->set_model(knifeModel);
                CGameSceneNode* pViewModelSceneNode = pHudModelWeapon->GetGameSceneNode();
                if (pViewModelSceneNode) {
                    pViewModelSceneNode->set_mesh_group_mask(1);
                }
                CGameSceneNode* pWeaponscene = pWeapon->GetGameSceneNode();
                if (pWeaponscene) {
                    pWeaponscene->set_mesh_group_mask(1);
                }
                pWeapon->UpdateComposite(1);
                pWeapon->UpdateCompositeSec(1);

            }
        }
    }



    void AddEconItemToList(CEconItem* pItem, float paintKit, float paintSeed, float paintWear, bool legacy)
    {
        if (!pItem)
            return;


        // записываем всё в список
        g_vecAddedItemsIDs.push_back({
            pItem->m_ulID,
            paintKit,
            paintSeed,
            paintWear,
            legacy
            });
    }


    void onEquipItemInLoadout(void* thisptr, int iTeam, int iSlot, uint64_t iItemID) {

        //std::cout << "[HOOK] EquipItemInLoadout called:" << std::endl;
        //std::cout << "  Team: " << iTeam << std::endl;
        //std::cout << "  Slot: " << iSlot << std::endl;
        //std::cout << "  ItemID: " << iItemID << std::endl;

        // Проверяем, есть ли этот предмет в списке добавленных предметов
        auto it = std::find_if(g_vecAddedItemsIDs.begin(), g_vecAddedItemsIDs.end(),
            [iItemID](const AddedItemInfo& info) {
                return info.id == iItemID;
            });

        if (it != g_vecAddedItemsIDs.end()) {

            // Записываем информацию об экипированном предмете в JSON файл
            if (InventoryPersistence::AppendEquippedItem(iItemID, iTeam, iSlot)) {
            }
            else {
                std::cout << "  [ERROR] Failed to save equipped item to JSON file" << std::endl;
            }
        }
        else {
            std::cout << "  [INFO] Item not found in added items list, skipping JSON save" << std::endl;
        }
    }

    void Shutdown() {


        CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
        if (!pInventory) return;

        for (const auto& item : g_vecAddedItemsIDs) {
            pInventory->RemoveEconItem(pInventory->GetSOCDataForItem(item.id));
        }

        // Очищаем список добавленных предметов
        g_vecAddedItemsIDs.clear();


        // Очищаем вектор скинов
        m_dumped_items.clear();
        // Очищаем вектор агентов
        agents.clear();
        // Очищаем вектор ножей
		knives.clear();
    }
}

