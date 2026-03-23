#pragma once
#include <string>
#include "fnv1a.h"
#include "memory.h"
#include "utlvector.h"

class CEconItem
{
	void SetDynamicAttributeValue(int index, void* value);
public:
	auto Destruct() { 
		return M::call_virtual<void>(this, 1 , true); 
	}

	static CEconItem* CreateInstance();
	void SetPaintKit(float kit)
	{
		SetDynamicAttributeValue(6, &kit);
	}
	void SetPaintSeed(float seed)
	{
		SetDynamicAttributeValue(7, &seed);
	}

	void SetPaintWear(float wear)
	{
		SetDynamicAttributeValue(8, &wear);
	}
	void SetStatTrak(int count)
	{
		SetDynamicAttributeValue(80, &count);
	}

	void SetStatTrakType(int type)
	{
		SetDynamicAttributeValue(81, &type);
	}
	void SetCustomName(const char* pName) {
		//	SetDynamicAttributeValueString(111, pName);
	}

	char pad0[0x10]; // 2 vtables
	uint64_t m_ulID;
	uint64_t m_ulOriginalID;
	void* m_pCustomDataOptimizedObject;
	uint32_t m_unAccountID;
	uint32_t m_unInventory;
	uint16_t m_unDefIndex;
	uint16_t m_unOrigin : 5;
	uint16_t m_nQuality : 4;
	uint16_t m_unLevel : 2;
	uint16_t m_nRarity : 4;
	uint16_t m_dirtybitInUse : 1;
	int16_t m_iItemSet;
	int m_bSOUpdateFrame;
	uint8_t m_unFlags;
};

class C_EconItemDefinition
{
public:
	C_EconItemDefinition* get_static_data() {
		return M::CallVFunc<C_EconItemDefinition*, 13>(this);
	}



	bool is_weapon() {
		if (get_stickers_supported_count() >= 4) return true;

		return false;
	}

	bool is_agent() {
		static auto type_custom_player = fnv1a::hash_64("#Type_CustomPlayer");
		auto hash_name = fnv1a::hash_64(m_pszItemTypeName);
		if (fnv1a::hash_64(m_pszItemTypeName) != type_custom_player)
			return false;
		return true;
		//return get_stickers_supported_count() >= 1;
	}

	bool is_knife(bool exclude_default) {
		static auto csgo_type_knife = fnv1a::hash_64("#CSGO_Type_Knife");
		if (fnv1a::hash_64(m_pszItemTypeName) != csgo_type_knife)
			return false;

		return exclude_default ? m_nDefIndex >= 500 : true;
	}

	bool is_glove(bool exclude_default) {
		static auto type_hands = fnv1a::hash_64("#Type_Hands");
		if (fnv1a::hash_64(m_pszItemTypeName) != type_hands)
			return false;

		const bool default_glove = m_nDefIndex == 5028 || m_nDefIndex == 5029;
		return exclude_default ? !default_glove : true;
	}

	bool is_weapon_case() {
		static auto type_weapon_case = fnv1a::hash_64("#CSGO_Type_WeaponCase");
		return fnv1a::hash_64(m_pszItemTypeName) == type_weapon_case;
	}

	bool is_key() {
		static auto tool_weapon_case_key_tag = fnv1a::hash_64("#CSGO_Tool_WeaponCase_KeyTag");
		return fnv1a::hash_64(m_pszItemTypeName) == tool_weapon_case_key_tag;
	}
	auto GetItemTypeName() {
		return *reinterpret_cast<const char**>((uintptr_t)(this) + 0x80);
	}

	uint8_t GetRarity()
	{
		return *reinterpret_cast<uint8_t*>((uintptr_t)(this) + 0x42);
	}

	const char* get_model_name() {
		return *reinterpret_cast<const char**>((uintptr_t)(this) + 0x148);
	}
	const char* get_simple() {
		return *reinterpret_cast<const char**>((uintptr_t)(this) + 0x260);
	}
	const char* get_model_name_only() {
		const char* full_path = *reinterpret_cast<const char**>((uintptr_t)(this) + 0x148);
		std::string path(full_path);
		size_t last_slash = path.find_last_of('/');
		std::string filename = (last_slash != std::string::npos) ? path.substr(last_slash + 1) : path;

		size_t dot = filename.find_last_of('.');
		if (dot != std::string::npos)
			filename = filename.substr(0, dot);

		static std::string result;
		result = filename;
		return result.c_str();
	}
	
	const char* GetSimpleWeaponName() {

		return *reinterpret_cast<const char**>((uintptr_t)(this) + 0x248);
	}

	auto GetName()
	{
		return *reinterpret_cast<const char**>((uintptr_t)(this) + 0x70);
	}

	auto GetLoadoutSlot()
	{
		return *reinterpret_cast<int*>((uintptr_t)(this) + 0x338);
	}


	int get_stickers_supported_count()
	{
		return *reinterpret_cast<int*>((uintptr_t)(this) + 0x168); // 0x118
	}

	char pad0[0x8]; // vtable
	void* m_pKVItem;
	uint16_t m_nDefIndex;
	CUtlVector<uint16_t> m_nAssociatedItemsDefIndexes;
	bool m_bEnabled;
	const char* m_szPrefab;
	uint8_t m_unMinItemLevel;
	uint8_t m_unMaxItemLevel;
	uint8_t m_nItemRarity;
	uint8_t m_nItemQuality;
	uint8_t m_nForcedItemQuality;
	uint8_t m_nDefaultDropItemQuality;
	uint8_t m_nDefaultDropQuantity;
	CUtlVector<void*> m_vecStaticAttributes;
	uint8_t m_nPopularitySeed;
	void* m_pPortraitsKV;
	const char* m_pszItemBaseName;
	bool m_bProperName;
	const char* m_pszItemTypeName;
	uint32_t m_unItemTypeID;
	const char* m_pszItemDesc;
};

struct C_PaintKit
{
	std::uint32_t m_id;   
	char pad0[4];
	const char* m_name;
	const char* m_description_string;
	const char* m_description_tag;
	std::byte pad_001[0x8];
	const char* m_pattern;
	const char* m_normal;
	const char* m_logo_material;
	std::byte pad_002[0x4];
	std::uint32_t m_rarity;
	std::uint32_t m_style;
	float m_rgba_color[4][4];
	float m_rgba_logo_color[4][4];
	float m_wear_default;
	float m_wear_remap_min;
	float m_wear_remap_max;
	std::uint8_t m_fixed_seed;
	std::uint8_t m_phong_exponent;
	std::uint8_t m_phong_albedo_boost;
	std::uint8_t m_phong_intensity;
	float m_pattern_scale;
	float m_pattern_offset_x_start;
	float m_pattern_offset_x_end;
	float m_pattern_offset_y_start;
	float m_pattern_offset_y_end;
	float m_pattern_rotate_start;
	float m_pattern_rotate_end;
	float m_logo_scale;
	float m_logo_offset_x;
	float m_logo_offset_y;
	float m_logo_rotation;
	bool m_ignore_weapon_size_scale;
	std::byte pad_003[0x3];
	std::uint32_t m_view_model_exponent_override_size;
	bool m_only_first_material;
	bool m_use_normal_model;
	bool m_use_legacy_model;
	std::byte pad_004[0x1];
	float m_pearlescent;
	const char* m_vmt_path;
	std::byte pad_005[0x8];
	const char* m_composite_material_path;
	void* m_vmt_overrides;
	std::byte pad_006[0x8];

	bool legacy() {


		return *(bool*)(std::uintptr_t(this) + 0xAE);


	}
};

class CSkiterKit {
public:
	int id;
	int rarity;
	const char* name;
	const char* description_name;
	const char* item_name;
	const char* material_path;
	const char* material_path_no_drips;
	const char* inventory_image;
	char pad[0x18];
	float rotate_start;
	float rotate_end;
	float scale_min;
	float scale_max;
	const char* path_image;
	void* unk;
	const char* path_image_large;
	char pad2[0x20];
};