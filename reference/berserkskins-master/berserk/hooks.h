#pragma once
#include "hook_manager.h"

// ============================================
// EXAMPLE FUNCTION SIGNATURES
// ============================================
class material_data_t;

// Example function types
typedef float(__fastcall* GetAsspectRatio)(void* thisptr, int width, int height);
typedef void(__fastcall* FrameStage)(void* _this, int curStage);
typedef void* (*EnableCursor)(void* rcx, bool active);
typedef bool(__thiscall* EquipItemInLoadout_t)(void* thisptr, int iTeam, int iSlot, uint64_t iItemID);
typedef void(__fastcall* oBatchList)(void* a1);
typedef std::int64_t(__fastcall* fnCreateMaterial)(void*, void*, const char*, void*, unsigned int, unsigned int);

typedef void(__fastcall* oDrawArray)(void* animtable_scene_object, void* dx11, material_data_t* data, int data_counter, void* scene_view, void* scene_layer, void* unknown_pointer);

typedef void(__fastcall* oLevelInit)(void* pClientModeShared, const char* szNewMap);

//48 89 54 24 ? 55 41 55
// ============================================
// DECLARE HOOK OBJECTS
// ============================================�

namespace H
{
     inline CBaseHookObject<EnableCursor> hkEnableCursor = {};
	 inline CBaseHookObject<EquipItemInLoadout_t> hkEquipItemInLoadout = {};

	 inline CBaseHookObject<FrameStage> hkFrameStage = {};
	 inline CBaseHookObject<GetAsspectRatio> hkGetAsspectRatio = {};

	 inline CBaseHookObject<fnCreateMaterial> hkCreateMaterial = {};


	 inline CBaseHookObject<oLevelInit> hkLevelInit = {};

}

// ============================================
// DETOUR FUNCTIONS
// ============================================


namespace D
{

	void hkBatchList(void* a1);
	void hkEnableCursor(void* rcx, bool active);
	void __fastcall GetScreenAspectRatio(void* thisptr, int width, int height);
	void __fastcall hkFrameStage(void* _this, int curStage);
	bool __fastcall hkEquipItemInLoadout(void* thisptr, int iTeam, int iSlot, uint64_t iItemID);

	void __fastcall hkLevelInit(void* pClientModeShared, const char* szNewMap);
}
