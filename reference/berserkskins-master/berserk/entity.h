#pragma once
#include "CEconItem.h"
#include "schema.h"
#include "memory.h"
#include "interfaces.h"
enum class CSWeaponCategory : std::uint32_t
{
	WEAPONCATEGORY_OTHER = 0x0,
	WEAPONCATEGORY_MELEE = 0x1,
	WEAPONCATEGORY_SECONDARY = 0x2,
	WEAPONCATEGORY_SMG = 0x3,
	WEAPONCATEGORY_RIFLE = 0x4,
	WEAPONCATEGORY_HEAVY = 0x5,
	WEAPONCATEGORY_COUNT = 0x6,
};
// Forward declarations
class CBaseHandle;
class C_EconItemDefinition;

// Constants for CBaseHandle
#define INVALID_EHANDLE_INDEX 0xFFFFFFFF
#define ENT_ENTRY_MASK 0x7FFF
#define NUM_SERIAL_NUM_SHIFT_BITS 15

// CBaseHandle definition
class CBaseHandle
{
public:
	CBaseHandle() noexcept :
		nIndex(INVALID_EHANDLE_INDEX) {
	}

	CBaseHandle(const int nEntry, const int nSerial) noexcept
	{
		nIndex = nEntry | (nSerial << NUM_SERIAL_NUM_SHIFT_BITS);
	}

	bool operator!=(const CBaseHandle& other) const noexcept
	{
		return nIndex != other.nIndex;
	}

	bool operator==(const CBaseHandle& other) const noexcept
	{
		return nIndex == other.nIndex;
	}

	bool operator<(const CBaseHandle& other) const noexcept
	{
		return nIndex < other.nIndex;
	}

	[[nodiscard]] bool IsValid() const noexcept
	{
		return nIndex != INVALID_EHANDLE_INDEX;
	}

	[[nodiscard]] int GetEntryIndex() const noexcept
	{
		return static_cast<int>(nIndex & ENT_ENTRY_MASK);
	}

	[[nodiscard]] int GetSerialNumber() const noexcept
	{
		return static_cast<int>(nIndex >> NUM_SERIAL_NUM_SHIFT_BITS);
	}

private:
	std::uint32_t nIndex;
};
template <typename T>
class c_network_utl_vector {
public:
	unsigned int m_size;
	T* m_elements;
};
// CGameEntitySystem definition
class CGameEntitySystem
{
public:
	/// GetClientEntity
	template <typename T = C_BaseEntity>
	T* Get(int nIndex)
	{
		return reinterpret_cast<T*>(this->GetEntityByIndex(nIndex));
	}

	/// GetClientEntityFromHandle
	template <typename T = C_BaseEntity>
	T* Get(const CBaseHandle hHandle)
	{
		if (!hHandle.IsValid())
			return nullptr;

		return reinterpret_cast<T*>(this->GetEntityByIndex(hHandle.GetEntryIndex()));
	}

	int GetHighestEntityIndex()
	{
		return *reinterpret_cast<int*>(reinterpret_cast<std::uintptr_t>(this) + 0x20F0);
	}
private:
	// Кэшированные адреса
	static uintptr_t cachedGameEntitySystem;
	static bool isGameEntitySystemCached;


	uintptr_t __fastcall GetEntityByIndex(int EntityId)
	{
		uintptr_t EntityBlock;
		uintptr_t Entity;

		// Используем кэшированный адрес, если он есть
		if (!isGameEntitySystemCached) {
			uintptr_t addr = (uintptr_t)M::FindPattern(L"client.dll", "48 8B 3D ? ? ? ? 48 89 3D ? ? ? ?");
			if (!addr) {
				printf("Pattern not found\n");
				return 0;
			}

			int32_t ripOffset = *(int32_t*)(addr + 3);
			uintptr_t gameEntitySystemPtr = addr + 7 + ripOffset;
			cachedGameEntitySystem = *(uintptr_t*)gameEntitySystemPtr;
			isGameEntitySystemCached = true;
		}

		uintptr_t GameEntitySystem = cachedGameEntitySystem;

		if ((unsigned int)EntityId <= 0x7FFE && (unsigned int)(EntityId >> 9) <= 0x3F) {
			EntityBlock = *(uintptr_t*)(GameEntitySystem + 8i64 * (EntityId >> 9) + 0x10);

			if (EntityBlock != 0) {
				uintptr_t entryAddr = 112 * (EntityId & 0x1FF) + EntityBlock;

				if (entryAddr < 0x10000 || entryAddr > 0x7FFFFFFFFFFF) {
					printf("Invalid entry address: %p\n", (void*)entryAddr);
					return 0;
				}

				Entity = *(uintptr_t*)(entryAddr);
				if (Entity != 0) {
					return Entity;
				}
			}
		}
		return 0;
	}
};

class CEntityInstance;
class CEntityIdentity
{
public:
	std::uint32_t GetIndex() const
	{
		return *reinterpret_cast<const std::uint32_t*>(
			reinterpret_cast<const std::uintptr_t>(this) + 0x10
			);
	}


	SCHEMA(const char*, GetDesignerName, "CEntityIdentity", "m_designerName");
	SCHEMA(std::uint32_t, GetFlags, "CEntityIdentity", "m_flags");

	[[nodiscard]] bool IsValid()
	{
		return GetIndex() != INVALID_EHANDLE_INDEX;
	}

	[[nodiscard]] int GetEntryIndex()
	{
		if (!IsValid())
			return ENT_ENTRY_MASK;

		return GetIndex() & ENT_ENTRY_MASK;
	}

	[[nodiscard]] int GetSerialNumber()
	{
		return GetIndex() >> NUM_SERIAL_NUM_SHIFT_BITS;
	}

	CEntityInstance* pInstance; // 0x00

};

class CEntityInstance
{
public:
	void GetSchemaClassInfo(SchemaClassFieldData_t** pReturn)
	{
		return M::CallVFunc<void, 38U>(this, pReturn);
	}
	SchemaClassInfoData_t* get_schema_class_info() {
		SchemaClassInfoData_t* class_info = nullptr;

		M::call_virtual<void>(this, 42, &class_info);

		return class_info;
	}

	const char* getClassName() {
		SchemaClassInfoData_t* class_info = get_schema_class_info();
		if (!class_info)
			return "asd";
		return class_info->get_name();
	}

	[[nodiscard]] CBaseHandle GetRefEHandle()
	{
		CEntityIdentity* pIdentity = GetIdentity();
		if (pIdentity == nullptr)
			return CBaseHandle();

		return CBaseHandle(pIdentity->GetEntryIndex(), pIdentity->GetSerialNumber() - (pIdentity->GetFlags() & 1));
	}

	SCHEMA(CEntityIdentity*, GetIdentity, "CEntityInstance", "m_pEntity");


};

// SCHEMA(Vec3, GetOrigin, "C_CSPlayerPawn", "m_vecOrigin");
// return player->GetOrigin();
// used: quaternion

class CSkeletonInstance;
class C_BaseEntity;
class CGameSceneNode
{
public:
	SCHEMA(C_BaseEntity*, m_pOwner, "CGameSceneNode", "m_pOwner");

	SCHEMA(CGameSceneNode*, m_pChild, "CGameSceneNode", "m_pChild");
	SCHEMA(CGameSceneNode*, m_pNextSibling, "CGameSceneNode", "m_pNextSibling");
	
	void set_mesh_group_mask(uint64_t meshGroupMask) {
		using function_t = void(__thiscall*)(void*, uint64_t);

		static function_t fn = reinterpret_cast<function_t>(M::FindPattern(L"client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71"));

		//if (!fn) {
		//	//	void* addr = memory::GetAbsoluteAddress(memory::PatternScan("client.dll", "E8 ? ? ? ? 48 8B CD E8 ? ? ? ? 4C 8B B4 24"), 0x1);
		//	void* addr = M::FindPattern(L"client.dll", "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8D 99 ? ? ? ? 48 8B 71");
		//	if (!addr)
		//		return;

		//	fn = reinterpret_cast<function_t>(addr);
		//	printf("[DEBUG] set_mesh_group_mask function found at %p\n", fn);
		//}

		fn(this, meshGroupMask);
	}
};


class CSkeletonInstance : public CGameSceneNode {
public:
	SCHEMA_EX(uintptr_t, boneArray, "CSkeletonInstance", "m_modelState", 0x80);
};


class CCollisionProperty
{
public:
	std::uint16_t CollisionMask()
	{
		return *reinterpret_cast<std::uint16_t*>(reinterpret_cast<std::uintptr_t>(this) + 0x38);
	}

	SCHEMA(std::uint8_t, GetSolidFlags, "CCollisionProperty", "m_usSolidFlags");
	SCHEMA(std::uint8_t, GetCollisionGroup, "CCollisionProperty", "m_CollisionGroup");

};
class CBaseHandle;
template <typename T>
class CHandle : public CBaseHandle {
public:
	T* Get() const { return static_cast<T*>(CBaseHandle::Get()); }
};

class C_BaseEntity : public CEntityInstance
{
private:
	static auto& get_function() {
		static auto fn = reinterpret_cast<void* (__fastcall*)(void*, const char*)>(
			M::FindPattern(L"client.dll", "40 53 48 83 EC ? 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24")		);

		if (!fn) {
			printf("[ERROR] set_model function pattern not found!\n");
		}

		return fn;
	}
public:
	SCHEMA(CBaseHandle, m_hOwnerEntity, "C_BaseEntity", "m_hOwnerEntity");
	SCHEMA(std::uint32_t, m_nSubclassID, "C_BaseEntity", "m_nSubclassID");

	SCHEMA(int, health, "C_BaseEntity", "m_iHealth");
	SCHEMA(int, team, "C_BaseEntity", "m_iTeamNum");
	SCHEMA(std::uint32_t, m_fFlags, "C_BaseEntity", "m_fFlags");

	SCHEMA(CGameSceneNode*, GetGameSceneNode, "C_BaseEntity", "m_pGameSceneNode");
	SCHEMA(CCollisionProperty*, GetCollision, "C_BaseEntity", "m_pCollision");
	SCHEMA_EX(void*, GetVData, "C_BaseEntity", "m_nSubclassID", 0x8);

	std::uint32_t GetOwnerHandle();


	bool is_weapon() {
		return M::call_virtual<bool>(this, 162);
	}
	void set_model(const char* model_name) {
		static auto fn = reinterpret_cast<void* (__fastcall*)(void*, const char*)>(M::FindPattern(L"client.dll", "40 53 48 83 EC ? 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24"));
		fn(this, model_name);

	}
	void UpdateSubClass()
	{
		using fnGetPlayerPawn = void(__fastcall*)(void*);
		static auto GetPlayerPawnf = reinterpret_cast<fnGetPlayerPawn>(M::FindPattern(L"client.dll", "40 53 48 83 EC 30 48 8B 41 10 48 8B D9 8B 50 30"));
		GetPlayerPawnf(this);
	}
	void UpdateVData()
	{
		M::CallVFunc<void*, 183>(this);
	}
	void SetBody_Group() {
		using function_t = void(__thiscall*)(void*, uint64_t, uint64_t);

		static function_t fn = reinterpret_cast<function_t>(M::FindPattern(L"client.dll", "85 D2 0F 88 ? ? ? ? 53 55"));

		fn(this, 0, 1);
	}


};
class C_BaseModelEntity : public C_BaseEntity
{
public:

};

class CPlayer_ObserverServices
{
public:
	SCHEMA(CBaseHandle, getTarget, "CPlayer_ObserverServices", "m_hObserverTarget");
};
class CPlayer_WeaponServices;

class CPlayer_CameraServices
{
public:
	SCHEMA(CBaseHandle, m_PostProcessingVolumes, "CPlayer_CameraServices", "m_PostProcessingVolumes");

};

class C_PostProcessingVolume
{
public:
	SCHEMA(bool, ExposureControl, "C_PostProcessingVolume", "m_bExposureControl");
	SCHEMA(float, MinExposure, "C_PostProcessingVolume", "m_flMinExposure");
	SCHEMA(float, MaxExposure, "C_PostProcessingVolume", "m_flMaxExposure");
	SCHEMA(float, FadeSpeedUp, "C_PostProcessingVolume", "m_flExposureFadeSpeedUp");
	SCHEMA(float, FadeSpeedDown, "C_PostProcessingVolume", "m_flExposureFadeSpeedDown");

};


// Example CS2 Player class using SCHEMA macros
class C_BasePlayerPawn : public C_BaseModelEntity
{
public:

	SCHEMA(CPlayer_ObserverServices*, GetObserverServices, "C_BasePlayerPawn", "m_pObserverServices");
	SCHEMA(CPlayer_WeaponServices*, GetWeaponServices, "C_BasePlayerPawn", "m_pWeaponServices");
	SCHEMA(CPlayer_CameraServices*, GetCameraServices, "C_BasePlayerPawn", "m_pCameraServices");




};

class CBaseAnimGraph : public C_BaseModelEntity
{
public:
};
class C_BaseFlex : public CBaseAnimGraph
{
public:
};
class C_EconItemView
{
public:
	SCHEMA(bool, m_bDisallowSOCm, "C_EconItemView", "m_bDisallowSOC");
	SCHEMA(int, m_iItemDefinitionIndex, "C_EconItemView", "m_iItemDefinitionIndex");
	SCHEMA(uint64_t, m_iItemID, "C_EconItemView", "m_iItemID");
	SCHEMA(uint32_t, m_iItemIDLow, "C_EconItemView", "m_iItemIDLow");
	SCHEMA(uint32_t, m_iItemIDHigh, "C_EconItemView", "m_iItemIDHigh");
	SCHEMA(uint32_t, m_iAccountID, "C_EconItemView", "m_iAccountID");
	SCHEMA(bool, m_bRestoreCustomMaterialAfterPrecache, "C_EconItemView", "m_bRestoreCustomMaterialAfterPrecache");
	SCHEMA(const char*, m_szCustomName, "C_EconItemView", "m_szCustomName");
	SCHEMA(bool, m_bInitialized, "C_EconItemView", "m_bInitialized");
	C_EconItemDefinition* get_static_data();

	void pCEconItemDescription() {
		*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x200) = 0;

	}
};
class C_CSGameRules {
public:
	SCHEMA(float, m_fRoundStartTime, "C_CSGameRules", "m_fRoundStartTime");

};
class C_AttributeContainer
{
public:
	SCHEMA(C_EconItemView, m_Item, "C_AttributeContainer", "m_Item");

};

class C_EconEntity : public C_BaseFlex
{
public:
	SCHEMA(C_AttributeContainer, m_AttributeManager, "C_EconEntity", "m_AttributeManager");
	SCHEMA(std::int32_t, GetFallbackPaintKit, "C_EconEntity", "m_nFallbackPaintKit");
	SCHEMA(std::int32_t, GetFallbackSeed, "C_EconEntity", "m_nFallbackSeed");
	SCHEMA(std::int32_t, GetFallbackWear, "C_EconEntity", "m_flFallbackWear");
	SCHEMA(std::uint32_t, GetOriginalOwnerXuidLow, "C_EconEntity", "m_OriginalOwnerXuidLow");
	SCHEMA(std::uint32_t, GetOriginalOwnerXuidHigh, "C_EconEntity", "m_OriginalOwnerXuidHigh");
	uint64_t GetOriginalOwnerXuid()
	{
		return ((uint64_t)(GetOriginalOwnerXuidHigh()) << 32) |
			GetOriginalOwnerXuidLow();
	}


};

class C_BasePlayerWeapon : public C_EconEntity
{
public:
	inline void* UpdateWeaponData()
	{
		return M::CallVFunc<void*, 199u>(this);
	}

	inline void* UpdateComposite(bool should)
	{
		return M::call_virtual<void*>(this,8, should);
	}

	inline void* UpdateCompositeSec(bool should)
	{
		return M::call_virtual<void*>(this, 105, should);
	}
};
class CPlayer_WeaponServices
{
public:
	SCHEMA(CBaseHandle, GetActiveWeapon, "CPlayer_WeaponServices", "m_hActiveWeapon");
	SCHEMA(c_network_utl_vector<CBaseHandle>, m_hMyWeapons, "CPlayer_WeaponServices", "m_hMyWeapons");
	SCHEMA(bool, m_bBlockInspectUntilNextGraphUpdate, "CCSPlayer_WeaponServices", "m_bBlockInspectUntilNextGraphUpdate");
};

enum ResourceType_t : uint8_t {
	RESOURCETYPE_BANK_GOLD = 0,
	RESOURCETYPE_INVENTORY_GOLD = 1,
	RESOURCETYPE_PREY_BONUS_REROLLS = 10,

	RESOURCETYPE_ALL = 255, // Just used internally
};
struct ResourceId_t
{
public:
	uint64_t m_Value; // 0x0	

	// Static fields:
};
#include "utlbuffer.h"
struct CResourceNameTyped {
	CBufferString m_szResourcePath;
	ResourceId_t m_nResourceId;      // 0xd0
	ResourceType_t m_nResourceType;  // 0xd8
};


class CCSWeaponBaseVData
{
public:

	SCHEMA(std::uint32_t, GetWeaponType, "CCSWeaponBaseVData", "m_WeaponType");
	SCHEMA(float, GetRange, "CCSWeaponBaseVData", "m_flRange");
	SCHEMA(CResourceNameTyped, m_szAnimSkeleton, "CCSWeaponBaseVData", "m_szAnimSkeleton")
	SCHEMA(CResourceNameTyped, m_szModel_AG2, "CCSWeaponBaseVData", "m_szModel_AG2")
	SCHEMA(float, m_flCycleTime, "CCSWeaponBaseVData", "m_flCycleTime");
	SCHEMA(std::int32_t, m_WeaponCategory, "CCSWeaponBaseVData", "m_WeaponCategory");

	SCHEMA(const char*, szName, "CCSWeaponBaseVData", "m_szName");

	// Recoil system
};
class C_CSWeaponBase : public C_BasePlayerWeapon
{
public:
	SCHEMA(int, m_iOriginalTeamNumber, "C_CSWeaponBase", "m_iOriginalTeamNumber");
	SCHEMA(bool, IsInReload, "C_CSWeaponBase", "m_bInReload");
	SCHEMA(CBaseHandle, m_hPrevOwner, "C_CSWeaponBase", "m_hPrevOwner");
	SCHEMA_EX(void*, m_hStattrakAttachment, "C_CSWeaponBase", "m_iNumEmptyAttacks", 4);
	SCHEMA(bool, m_bUIWeapon, "C_CSWeaponBase", "m_bUIWeapon");
	SCHEMA_EX(void*, m_hNametagAttachment, "C_CSWeaponBase", "m_iNumEmptyAttacks", 20);
	SCHEMA(int, m_weaponMode, "C_CSWeaponBase", "m_weaponMode");

	SCHEMA(bool, m_bInspectPending, "C_CSWeaponBase", "m_bInspectPending");
	SCHEMA(bool, m_bInspectShouldLoop, "C_CSWeaponBase", "m_bInspectShouldLoop");

	SCHEMA(float, m_flInspectCancelCompleteTime, "C_CSWeaponBase", "m_flInspectCancelCompleteTime");

	

	// Recoil system methods

	CCSWeaponBaseVData* GetWeaponVData()
	{
		return static_cast<CCSWeaponBaseVData*>(GetVData());
	}

	void AddStattrakEntity() {
		auto mItem = &this->m_AttributeManager().m_Item();
		using function_t = char(__fastcall*)(void* a1, void* a2);

		static function_t fn = reinterpret_cast<function_t>(
			M::FindPattern(L"client.dll",
				"48 89 5C 24 ? 48 89 74 24 ? 55 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B DA")
			);
		if (fn)
			fn(this, mItem);
		else
			printf("error\n");
		//debug(hooks::m_add_name_tag_entity != nullptr);
	}


	void AddNametagEntity() {
		auto mItem = &this->m_AttributeManager().m_Item();
		using function_t = char(__fastcall*)(void* a1, void* a2);

		static function_t fn = reinterpret_cast<function_t>(
			M::FindPattern(L"client.dll",
				"40 55 53 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B DA 48 8B F1 48 85 C9")
			);
		if (fn)
			fn(this, mItem);
		else
			printf("error\n");
		//debug(hooks::m_add_name_tag_entity != nullptr);
	}


	//// Advanced recoil system methods
	//Vec3 get_recoil_offset(int recoil_index);
	//Vec3 simulate_aimpunch(int recoil_index);
	CCSWeaponBaseVData* get_vdata() { return GetWeaponVData(); }

};

class C_CSPlayerPawnBase : public C_BasePlayerPawn
{
public:
	SCHEMA(float, m_flLastSpawnTimeIndex, "C_CSPlayerPawnBase", "m_flLastSpawnTimeIndex");

	SCHEMA(float, GetFlashDuration, "C_CSPlayerPawnBase", "m_flFlashDuration");
};
class C_BaseModelEntity;
class C_CS2HudModelArms : public CBaseAnimGraph
{
public:


};
#include "utlvector.h"
class C_CS2HudModelWeapon : public CBaseAnimGraph
{
public:

};
struct QAngle_t;
class C_CSPlayerPawn : public C_CSPlayerPawnBase
{
public:
	[[nodiscard]] std::uint32_t GetOwnerHandleIndex();
	[[nodiscard]] std::uint16_t GetCollisionMask();

	static C_CSPlayerPawn* GetLocalPawn();
	SCHEMA(float, m_flViewmodelFOV, "C_CSPlayerPawn", "m_flViewmodelFOV")
		SCHEMA(QAngle_t, m_aimPunchAngle, "C_CSPlayerPawn", "m_aimPunchAngle");
	SCHEMA(QAngle_t, m_angEyeAngles, "C_CSPlayerPawn", "m_angEyeAngles");
	SCHEMA(bool, isScoped, "C_CSPlayerPawn", "m_bIsScoped");
	SCHEMA(C_CSWeaponBase*, GetClippingWeapon, "C_CSPlayerPawnBase", "m_pClippingWeapon")
		SCHEMA(CBaseHandle, m_hHudModelArms, "C_CSPlayerPawn", "m_hHudModelArms");
	SCHEMA(C_EconItemView, m_EconGloves, "C_CSPlayerPawn", "m_EconGloves");
	SCHEMA(int, m_iShotsFired, "C_CSPlayerPawn", "m_iShotsFired")
		SCHEMA(bool, m_bNeedToReApplyGloves, "C_CSPlayerPawn", "m_bNeedToReApplyGloves");
	C_CSWeaponBase* GetActiveWeaponPlayer();
	std::vector<C_CS2HudModelWeapon*> GetViewModels();


	C_CS2HudModelWeapon* GetViewModel();
	void SetBodyGroup() {
		using function_t = void(__thiscall*)(void*, uint64_t, uint64_t);

		static function_t fn = nullptr;

		if (!fn) {
			//	void* addr = memory::GetAbsoluteAddress(memory::PatternScan("client.dll", "E8 ? ? ? ? 48 8B CD E8 ? ? ? ? 4C 8B B4 24"), 0x1);
			void* addr = M::FindPattern(L"client.dll", "85 D2 0F 88 ? ? ? ? 53 55");
			if (!addr)
				return;

			fn = reinterpret_cast<function_t>(addr);
			printf("[DEBUG] SetBodyGroup function found at %p\n", fn);
		}

		//for (int a = 0; a <= 1; ++a)
		//{
		//	for (int b = 0; b <= 1; ++b)
		//	{
		//		fn(this, a, b);
		//	}
		//}
		fn(this, 0, 1);
	}


};

class CBasePlayerController : public C_BaseModelEntity
{
public:
	SCHEMA(CBaseHandle, GetPawnHandle, "CBasePlayerController", "m_hPawn");

};

class c_user_cmd_manager;
class CUserCmd;
class CCSPlayerController : public CBasePlayerController
{
public:

	static CCSPlayerController* GetLocalPlayerController();
	SCHEMA(bool, IsPawnAlive, "CCSPlayerController", "m_bPawnIsAlive");


	SCHEMA(int, GetPing, "CCSPlayerController", "m_iPing");
	SCHEMA(const char*, GetPlayerName, "CCSPlayerController", "m_sSanitizedPlayerName");


};



#define INVALID_EHANDLE_INDEX 0xFFFFFFFF
#define ENT_ENTRY_MASK 0x7FFF
#define NUM_SERIAL_NUM_SHIFT_BITS 15
// @source: https://developer.valvesoftware.com/wiki/Entity_limit#Source_2_limits
#define ENT_MAX_NETWORKED_ENTRY 16384


