#include "hook_manager.h"
#include "logger.h"
#include "hooks.h"
#include "memory.h"
#include "sdk.h"
#include "interfaces.h"
#include "entity.h"
#include "skins.h"
#include "CCPlayerInventory.h"
#include "color.hpp"
#include "i_materialsystem.h"

namespace H
{
	bool Setup()
	{
		if (const MH_STATUS status = MH_Initialize(); status != MH_OK)
		{
			printf("[H] Failed to initialize MinHook: %s\n", MH_StatusToString(status));
			return false;
		}

		if (!hkGetAsspectRatio.Create(reinterpret_cast<void*>(M::FindPattern(L"engine2.dll", "48 89 5C 24 08 57 48 83 EC 20 8B FA 48 8D 0D")), &D::GetScreenAspectRatio))
		{
			printf("[---] Failed to create GetScreenAspectRatio hook\n");
			return false;
		}
		if (!hkFrameStage.Create(M::get_v_method(I::iclient, 36), &D::hkFrameStage)) {
			printf("[---] Failed to create hkFrameStage hook\n");
			return false;
		}
		if (!hkEnableCursor.Create(M::get_v_method(I::inputsystem, 76), &D::hkEnableCursor))
		{
			printf("[---] Failed to create EnableCursor hook\n");
			return false;
		}

		if (!hkEquipItemInLoadout.Create(reinterpret_cast<void*>(M::FindPattern(L"client.dll","48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 89 54 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 0F B7 F2" )), &D::hkEquipItemInLoadout)) 
		{
			printf("[---] Failed to create hkEquipItemInLoadout hook!\n");
		}


		if (!hkLevelInit.Create(reinterpret_cast<void*>(M::FindPattern(L"client.dll", "40 55 56 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48")), &D::hkLevelInit)) {
			printf("[---] Failed to create hkLevelInit hook!\n");
		}
		return true;
	}

	void Destroy()
	{
		printf("[H] Destroying hooks...\n");


		hkCreateMaterial.Remove();
		hkGetAsspectRatio.Remove();
		hkEquipItemInLoadout.Remove();
		hkEnableCursor.Remove();
		hkFrameStage.Remove();
		if (const MH_STATUS status = MH_Uninitialize(); status != MH_OK)
		{
			printf("[---] Failed to uninitialize MinHook: %s\n", MH_StatusToString(status));
			return;
		}

		printf("[H] MinHook uninitialized successfully\n");
	}
}


namespace D
{
	void __fastcall hkLevelInit(void* pClientModeShared, const char* szNewMap)
	{
		printf("[H] LevelInit called for map: %s\n", szNewMap);
		I::pvs->Set(false);
		H::hkLevelInit.GetOriginal()(pClientModeShared, szNewMap);
	}



	void __fastcall GetScreenAspectRatio(void* thisptr, int width, int height) {

		if (SDK::g_ScreenHeight != height || SDK::g_ScreenWidth != width) {
			SDK::g_ScreenHeight = height;
			SDK::g_ScreenWidth = width;
			printf("[D] Screen resized: width=%d, height=%d\n", width, height);
		}
		H::hkGetAsspectRatio.GetOriginal()(thisptr, width, height);

	}
	void __fastcall hkFrameStage(void* _this, int curStage)
	{
		//  printf("hk %d\n", curStage);
		H::hkFrameStage.GetOriginal()(_this, curStage);
		if (!I::engine && !I::engine->is_in_game()) return;
		if (curStage == 6) {



			C_CSPlayerPawn* local_pawn = C_CSPlayerPawn::GetLocalPawn();
			if (!local_pawn) return;
			CCSPlayerInventory* inventory = CCSInventoryManager::GetInstance()->GetLocalInventory();

			C_CS2HudModelWeapon* viewModel = local_pawn->GetViewModel();
			S::set_agent(curStage);
			S::ApplyKnifeSkins(local_pawn, inventory, viewModel);
			S::ApplyWeaponSkins(local_pawn, inventory, viewModel);
			S::ApplyGloves();
			//skins::set_knife(curStage);
		}
	}

	//void hkBatchList(void* a1)
	//{
	//	void** SceneObjectDesc = *reinterpret_cast<void***>(reinterpret_cast<uintptr_t>(a1) + 0x38);

	//	if (SceneObjectDesc) {
	//		const char* something = reinterpret_cast<const char* (__fastcall*)(void*)>(*reinterpret_cast<void***>(SceneObjectDesc)[0])(SceneObjectDesc);
	//		auto classname = std::string_view(something);
	//		auto v10 = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(a1) + 0x18);


	//		uint32_t count = *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(a1) + 0x30);

	//		if (v10)
	//		{
	//			for (uint32_t i = 0; i < count; ++i)
	//			{
	//				CMeshDrawPrimitive_t* meshDraw = (CMeshDrawPrimitive_t*)(reinterpret_cast<uintptr_t>(v10) + (0x68 * i));
	//

	//				if (!meshDraw)
	//					continue;
	//				if (classname.find("CParticleObjectDesc") != std::string::npos) {
	//					meshDraw->m_rgba = Color_t(255, 255, 255);;

	//				}
	//				if (classname.find("CAnimatableSceneObjectDesc") != std::string::npos) {
	//					Color_t col1(255, 0, 0);
	//					Color_t col2(255, 255, 255);
	//					if (!meshDraw->m_pObject)
	//						continue;

	//					auto pEntity = I::gameresource->pGameEntitySystem->Get<C_BaseEntity>(meshDraw->m_pObject->m_owner);
	//					if (pEntity == nullptr)
	//						continue;
	//					bool is_player = false;
	//					bool is_other = false;
	//					bool is_arm = false;
	//					bool is_view_model = false;

	//					int team = pEntity->team();
	//					int localteam = C_CSPlayerPawn::GetLocalPawn()->team();
	//					int health = pEntity->health();
	//					if (health > 0) {
	//						meshDraw->m_pMaterial = array_materials[VISUAL_MATERIAL_GLOW].m_material_z;
	//						meshDraw->m_rgba = col2;

	//					}

	//				}
	//				//// Particles, e.g. molotov
	//				//if (classname.find("CParticleObjectDesc") != std::string::npos)
	//				//{
	//				//	meshDraw->m_rgba = col1;
	//				//}

	//				//// World modulation
	//				//if (classname.find("CAnimatableSceneObjectDesc") != std::string::npos)
	//				//{
	//				//	auto materialName = std::string_view(meshDraw->m_material->GetName());
	//				//	if (materialName.find("characters") != std::string::npos)
	//				//	{
	//				//		meshDraw->m_material = array_materials[VISUAL_MATERIAL_GLOW].m_material_z;
	//				//		meshDraw->m_color = col2;
	//				//		H::hkBatchList.GetOriginal()(a1);
	//				//		meshDraw->m_material = array_materials[VISUAL_MATERIAL_GLOW].m_material;
	//				//		meshDraw->m_color = col1;
	//				//		H::hkBatchList.GetOriginal()(a1);
	//				//	}



	//				//}

	//			/*	if (classname.find("CBaseSceneObjectDesc") != std::string::npos)
	//				{
	//					meshDraw->m_rgba = col1;
	//				}

	//				if (classname.find("CAggregateSceneObjectDesc") != std::string::npos)
	//				{
	//					if (meshDraw->m_pObject)
	//					{
	//						for (int j = 0; j < meshDraw->m_pObject->count; ++j)
	//						{
	//							CAggregateSceneObjectData& data = meshDraw->m_pObject->data[j];
	//							data.m_rgba = col1;
	//						}
	//					}
	//				}*/
	//			}
	//		}
	//	}
	//	H::hkBatchList.GetOriginal()(a1);

	////}

	void hkEnableCursor(void* thisptr, bool bEnable)
	{
		if (!SDK::showMenu) {
			SDK::cursorlaststate = bEnable;
		}
		H::hkEnableCursor.GetOriginal()(thisptr, bEnable);
	}

	bool __fastcall hkEquipItemInLoadout(void* thisptr, int iTeam, int iSlot, uint64_t iItemID) {

		// Вызываем оригинальную функцию
		bool result = H::hkEquipItemInLoadout.GetOriginal()(thisptr, iTeam, iSlot, iItemID);
		S::onEquipItemInLoadout(thisptr, iTeam, iSlot, iItemID);

		return result;
	}

	

}