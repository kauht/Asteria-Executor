#include "entity.h"
#include <cmath>
// Определение статических переменных для кэширования
uintptr_t CGameEntitySystem::cachedGameEntitySystem = 0;
bool CGameEntitySystem::isGameEntitySystemCached = false;


CCSPlayerController* CCSPlayerController::GetLocalPlayerController()
{


	const int nIndex = I::engine->get_local_player_index();

	// Проверяем, что индекс валидный
	if (nIndex <= 0) {
		//printf("[ERROR] Invalid player index: %d\n", nIndex);
		return nullptr;
	}

	CCSPlayerController* result = I::gameresource->pGameEntitySystem->Get<CCSPlayerController>(nIndex);

	return result;
}

C_CSPlayerPawn* C_CSPlayerPawn::GetLocalPawn()
{
	CCSPlayerController* localController = CCSPlayerController::GetLocalPlayerController();

	if (localController == nullptr)
		return nullptr;

	CBaseHandle hPawnHandle = localController->GetPawnHandle();

	if (!hPawnHandle.IsValid())
		return nullptr;

	C_CSPlayerPawn* pPlayerPawn = I::gameresource->pGameEntitySystem->Get<C_CSPlayerPawn>(hPawnHandle);
	if (pPlayerPawn == nullptr)
		return nullptr;


	return pPlayerPawn;
}

C_CSWeaponBase* C_CSPlayerPawn::GetActiveWeaponPlayer()
{
	CPlayer_WeaponServices* pWeaponServices = this->GetWeaponServices();
	if (pWeaponServices == nullptr)
		return nullptr;

	if (!pWeaponServices->GetActiveWeapon().IsValid())
		return nullptr;

	return I::gameresource->pGameEntitySystem->Get<C_CSWeaponBase>(pWeaponServices->GetActiveWeapon());
}

std::vector<C_CS2HudModelWeapon*> C_CSPlayerPawn::GetViewModels() {
	CBaseHandle hud = this->m_hHudModelArms(); // m_hHudModelArms

	// Проверяем валидность handle
	if (!hud.IsValid()) {
		return {};
	}

	C_CS2HudModelArms* pViewArms = I::gameresource->pGameEntitySystem->Get<C_CS2HudModelArms>(hud); // m_hHudModelArms
	if (!pViewArms) {
		return {};
	}

	CGameSceneNode* pGameSceneNode = pViewArms->GetGameSceneNode();
	if (!pGameSceneNode) {
		return {};
	}

	std::vector<C_CS2HudModelWeapon*> vecViewModels = {};
	for (CGameSceneNode* pChild = pGameSceneNode->m_pChild(); pChild; pChild = pChild->m_pNextSibling()) {
		C_BaseEntity* pOwner = reinterpret_cast<C_BaseEntity*>(pChild->m_pOwner());
		if (!pOwner) {
			continue;
		}
		//printf("pOwner: %s", pOwner->getClassName());
		if (strcmp(pOwner->getClassName(), "C_CS2HudModelWeapon") == 0) {
			vecViewModels.push_back(reinterpret_cast<C_CS2HudModelWeapon*>(pOwner));
		}
	}

	return vecViewModels;
}
C_EconItemDefinition* C_EconItemView::get_static_data() {
	return M::call_virtual<C_EconItemDefinition*>(this, 13);
}



C_CS2HudModelWeapon* C_CSPlayerPawn::GetViewModel() {
	//printf("[TRACE] Entered GetViewModel for %s\n", this->getClassName());

	C_CSWeaponBase* pWeapon = C_CSPlayerPawn::GetActiveWeaponPlayer(); // CPlayer_WeaponServices -> m_hActiveWeapon
	if (!pWeapon) {
		return nullptr;
	}

	std::vector<C_CS2HudModelWeapon*> vecViewModels = C_CSPlayerPawn::GetViewModels();
	//printf("[TRACE] vecViewModels.size() = %zu\n", vecViewModels.size());
	if (vecViewModels.empty()) {
		//printf("[ERROR] No view models found for player %s\n", this->getClassName());
		return nullptr;
	}


	for (C_CS2HudModelWeapon* pViewModel : vecViewModels) {
		auto pOwner = I::gameresource->pGameEntitySystem->Get<C_BaseEntity>(pViewModel->m_hOwnerEntity()); // m_hHudModelArms


		//printf("[TRACE] ViewModel: %s (%p), Owner: %p, Weapon: %p\n",
		//	pViewModel->getClassName(),
		//	pViewModel,
		//	pOwner,
		//	pWeapon);

		if (!pOwner) {
			//printf("[WARN] ViewModel %p has no owner!\n", pViewModel);
			continue;
		}


		if (pOwner == pWeapon) {
			//printf("[DEBUG] Found view model for %s: %s: %s\n",
				//pOwner->getClassName(),
				//pViewModel->getClassName(),
				//pWeapon->getClassName());
			return pViewModel;
		}
	}

	return nullptr;
}

std::uint32_t C_CSPlayerPawn::GetOwnerHandleIndex()
{
	std::uint32_t Result = -1;
	if (this && GetCollision() && !(GetCollision()->GetSolidFlags() & 4))
		Result = this->m_hOwnerEntity().GetEntryIndex();

	return Result;
}

std::uint16_t C_CSPlayerPawn::GetCollisionMask()
{
	if (this && GetCollision())
		return GetCollision()->CollisionMask(); // Collision + 0x38

	return 0;
}

std::uint32_t C_BaseEntity::GetOwnerHandle()
{
	std::uint32_t entry_index = -1;

	if (this && GetCollision() && !(GetCollision()->GetSolidFlags() & 0x0004))
		entry_index = this->m_hOwnerEntity().GetEntryIndex();

	return entry_index;
}
