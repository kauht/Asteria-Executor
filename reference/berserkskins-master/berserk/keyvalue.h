#pragma once
#include "memory.h"
#include "utlbuffer.h"
struct KV3ID_t
{
	const char* szName;
	std::uint64_t unk0;
	std::uint64_t unk1;
};
const KV3ID_t g_KV3Format_Generic = { "generic", 0x469806E97412167C, 0xE73790B53EE6F2AF };
class KeyValues
{
public:
	std::byte pad01[0x100];
	std::uint64_t uKey;
	void* pValue;
	std::byte pad02[0x8];

	static bool LoadKV3(KeyValues* kv, const char* material_vmat, const char* kv_name)
	{
		using fn = bool(__fastcall*)(void* thisptr, void* utlstring, const char* buffer, const KV3ID_t* format, const char* kv_name);
		const void* hTier0 = M::GetModuleBaseHandle(L"tier0.dll");

		const auto addr = M::GetExportAddress(hTier0, "?LoadKV3@@YA_NPEAVKeyValues3@@PEAVCUtlString@@PEBDAEBUKV3ID_t@@2I@Z");
		if (!addr)
			return false;

		auto load_kv3 = reinterpret_cast<fn>(addr);

		return load_kv3(kv, nullptr, material_vmat, &g_KV3Format_Generic, kv_name);
	}
};