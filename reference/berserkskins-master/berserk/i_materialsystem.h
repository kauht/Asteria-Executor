#pragma once
#include "entity.h"
#include "memory.h"
#include "color.hpp"
class material2_t {
public:
	virtual const char* get_name() = 0;
	virtual const char* get_shader_name() = 0;
};

struct material_key_var_t {
	std::uint64_t m_key;
	const char* m_name;

	material_key_var_t(std::uint64_t m_key, const char* m_name) :
		m_key(m_key), m_name(m_name) {
	}

	material_key_var_t(const char* m_name, bool m_should_find_key = false) :
		m_name(m_name) {
		m_key = m_should_find_key ? find_key(m_name) : 0x0;
	}

	// find key
	std::uint64_t find_key(const char* m_name) {
		// helper ida: CBodyGameSystem::NotifyResourcePreReload
		using function_find_key = std::uint64_t(__fastcall*)
			(const char*, unsigned int, int);
		static auto find_key_var = reinterpret_cast<function_find_key>(
			M::FindPattern(L"particles.dll", "48 89 5C 24 ? 57 48 81 EC ? ? ? ? 33 C0 8B DA"));


		return find_key_var(m_name, 0x12, 0x31415926);
	}
};

// object info.
class object_info_t {
	char padd[0xB0];
	int m_id;
};
class scene_animable_object_t {
public:
	char padd[0xB8];
	CBaseHandle m_owner;
};
class CMaterial2;

class material_data_t {
public:
	void set_shader_type(const char* szShaderName)
	{
		// @ida: #STR: shader, spritecard.vfx
		using fnSetMaterialShaderType = void(__fastcall*)(void*, material_key_var_t, const char*, int);
		static auto oSetMaterialShaderType = reinterpret_cast<fnSetMaterialShaderType>(M::FindPattern(L"particles.dll", "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 0F B6 01 45 0F B6 F9 8B 2A 4D 8B E0 4C 8B 72 ? 48 8B F9 C0 E8 ? 24 ? 3C ? 74 ? 41 B0 ? B2 ? E8 ? ? ? ? 0F B6 07 33 DB C0 E8 ? 24 ? 3C ? 75 ? 48 8B 77 ? EB ? 48 8B F3 4C 8D 44 24 ? C7 44 24 ? ? ? ? ? 48 8D 54 24 ? 89 6C 24 ? 48 8B CE 4C 89 74 24 ? E8 ? ? ? ? 8B D0 83 F8 ? 75 ? 45 33 C9 89 6C 24 ? 4C 8D 44 24 ? 4C 89 74 24 ? 48 8B D7 48 8B CE E8 ? ? ? ? 8B D0 0F B6 0F C0 E9 ? 80 E1 ? 80 F9 ? 75 ? 48 8B 4F ? EB ? 48 8B CB 8B 41 ? 85 C0 74 ? 48 8D 59 ? 83 F8 ? 76 ? 48 8B 1B 48 63 C2 4D 85 E4"));


		material_key_var_t shaderVar(0x162C1777, "shader");
		oSetMaterialShaderType(this, shaderVar, szShaderName, 0x18);
	}
	void set_material_function(const char* szFunctionName, int nValue)
	{
		using fnSetMaterialFunction = void(__fastcall*)(void*, material_key_var_t, int, int);
		static auto oSetMaterialFunction = reinterpret_cast<fnSetMaterialFunction>(M::FindPattern(L"particles.dll", "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 0F B6 01 45 0F B6 F9 8B 2A 48 8B F9"));



		material_key_var_t functionVar(szFunctionName, true);
		oSetMaterialFunction(this, functionVar, nValue, 0x18);
	}
	char padd[0x18];

	scene_animable_object_t* m_scene_animable; // 0x18
	CMaterial2* m_material; // 0x20
	CMaterial2* m_materialcopy; // 0x20

	char padd1[0x10];
	object_info_t* pObjectInfo;
	char padd2[0x8];
	Color_t m_color; // 0x40
};






class CAggregateSceneObjectData {
public:
	char padd[0x38];
	Color_t m_rgba;
	char padd1[0x8];
};

class CAggregateSceneObject
{
public:
	// 0x000 — неизвестные данные
	char pad0[0xB8];               // совпадает со scene_animable_object_t::padd

	// 0x0B8 — владелец
	CBaseHandle m_owner;           // из scene_animable_object_t

	// 0x0C0 — неизвестные данные до count
	char pad1[0x120 - 0xC0];       // выравнивание до смещения 0x120

	// 0x120
	int count;

	// 0x124
	char pad2[0x4];

	// 0x128
	CAggregateSceneObjectData* data;
};


class CMeshDrawPrimitive_t {
public:
	char padd5[0x18];
	CAggregateSceneObject* m_pObject;
	CMaterial2* m_pMaterial;
	char padd6[0x28];
	Color_t m_rgba;
};