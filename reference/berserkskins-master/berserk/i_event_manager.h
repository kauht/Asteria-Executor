#pragma once
#include <cstdint>
#include <corecrt_malloc.h>
#include <string.h>
#include <algorithm>
#include "memory.h"
#define STRINGTOKEN_MURMURHASH_SEED 0x31415926 
class CGameEvent;

static uint32_t MurmurHash2(const void* key, int len, uint32_t seed)
{
	const uint32_t m = 0x5bd1e995;
	const int r = 24;

	uint32_t h = seed ^ len;

	const unsigned char* data = (const unsigned char*)key;

	while (len >= 4)
	{
		uint32_t k = *(uint32_t*)data;

		k *= m;
		k ^= k >> r;
		k *= m;

		h *= m;
		h ^= k;

		data += 4;
		len -= 4;
	}

	switch (len)
	{
	case 3:
		h ^= data[2] << 16;
	case 2:
		h ^= data[1] << 8;
	case 1:
		h ^= data[0];
		h *= m;
	};

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

#define TOLOWERU(c) ((uint32_t)(((c >= 'A') && (c <= 'Z')) ? c + 32 : c))

static uint32_t MurmurHash2LowerCaseA(const char* pString, int len, uint32_t nSeed)
{
	char* p = (char*)malloc(len + 1);
	for (int i = 0; i < len; i++)
	{
		p[i] = TOLOWERU(pString[i]);
	}
	return MurmurHash2(p, len, nSeed);
}

class cUltStringToken
{
public:
	std::uint32_t m_nHashCode;

	cUltStringToken(const char* szString)
	{
		this->SetHashCode(this->MakeStringToken(szString));
	}

	bool operator==(const cUltStringToken& other) const
	{
		return (other.m_nHashCode == m_nHashCode);
	}

	bool operator!=(const cUltStringToken& other) const
	{
		return (other.m_nHashCode != m_nHashCode);
	}

	bool operator<(const cUltStringToken& other) const
	{
		return (m_nHashCode < other.m_nHashCode);
	}

	uint32_t GetHashCode(void) const
	{
		return m_nHashCode;
	}

	void SetHashCode(uint32_t nCode)
	{
		m_nHashCode = nCode;
	}

	__forceinline std::uint32_t MakeStringToken(const char* szString, int nLen)
	{
		std::uint32_t nHashCode = MurmurHash2LowerCaseA(szString, nLen, STRINGTOKEN_MURMURHASH_SEED);
		return nHashCode;
	}

	__forceinline std::uint32_t MakeStringToken(const char* szString)
	{
		return MakeStringToken(szString, (int)strlen(szString));
	}

	cUltStringToken()
	{
		m_nHashCode = 0;
	}
};

class CBuffer
{
public:
	char Pad[0x8];
	const char* name;
};
class CCSPlayerController;
class CGameEventHelper
{
public:
	CGameEventHelper(CGameEvent* Event) :
		Event{ std::move(Event) } {
	};
	CCSPlayerController* GetPlayerController();
	CCSPlayerController* GetAttackerController();
	int GetDamage();

protected:
	CGameEvent* Event;
};
class CGameEvent
{
public:
	virtual ~CGameEvent() = 0;
	virtual const char* GetName() const = 0;
	virtual void Unk1() = 0;
	virtual void Unk2() = 0;
	virtual void Unk3() = 0;
	virtual void Unk4() = 0;
	virtual bool GetBool(const char* KeyName = nullptr, bool DefaultValue = false) = 0;
	virtual int GetInt(CBuffer* KeyName = nullptr, bool DefaultValue = false) = 0;
	virtual uint64_t GetUint64(const char* KeyName = nullptr, uint64_t DefaultValue = 0) = 0;
	virtual float GetFloat(const char* KeyName = nullptr, float DefaultValue = 0.f) = 0;
	virtual const char* GetString(const cUltStringToken& KeyName = nullptr, const char* DefaultValue = "") = 0;
	virtual void Unk5() = 0;
	virtual void Unk6() = 0;
	virtual void Unk7() = 0;
	virtual void Unk8() = 0;
	virtual void GetControllerId(int& ControllerId, CBuffer* Buffer = nullptr) = 0;
	virtual void Unk9() = 0;   // index 16
	virtual void Unk10() = 0;  // index 17
	virtual void Unk11() = 0;  // index 18
	virtual void Unk12() = 0;  // index 19
	virtual void SetBool(const char* KeyName, bool Value) = 0;    // index 20
	virtual void SetInt(const char* KeyName, int Value) = 0;     // index 21
	virtual void SetUint64(const char* KeyName, uint64_t Value) = 0;  // index 22
	virtual void SetFloat(const char* KeyName, float Value) = 0;      // index 23
	virtual void SetString(const cUltStringToken& KeyName = nullptr, const char* value = nullptr) = 0; // index 24

public:
	void SetStringA(CBuffer* Buffer, const cUltStringToken& Value)
	{
		M::CallVFunc<void, 24>(this, Buffer, Value);
	}

	CGameEventHelper GetEventHelper()
	{
		CGameEventHelper EventHelper(this);
		return EventHelper;
	}

	int GetInt2(const char* Name, bool Unk);
	//float GetFloat2(const char* Name, bool Unk);

	[[nodiscard]] float GetFloatNew(const cUltStringToken& keyToken, const float flDefault = 0.0f) const
	{
		using function_t = float(__thiscall*)(const void*, const cUltStringToken&, float);
		auto vtable = *reinterpret_cast<function_t**>(const_cast<void*>(static_cast<const void*>(this)));
		return vtable[9](this, keyToken, flDefault);
	}
};


class IGameEventListener
{
public:
	virtual ~IGameEventListener() {}

	virtual void FireGameEvent(CGameEvent* Event) = 0;
};

class i_event_manager
{
public:

	int LoadEventsFromFile(const char* szFileName)
	{
		return M::CallVFunc<int, 1U>(this, szFileName);
	}

	void Reset()
	{
		M::CallVFunc<void, 2U>(this);
	}

	bool AddListener(IGameEventListener* pListener, const char* szName, bool bServerSide = false)
	{
		return M::call_virtual<bool>(this, 3, pListener, szName, bServerSide);
	}

	bool FindListener(IGameEventListener* pListener, const char* szName)
	{
		return M::call_virtual<bool>(this,4, pListener, szName);
	}

	void RemoveListener(IGameEventListener* pListener)
	{
		M::call_virtual<void>(this, 5,pListener);
	}

};

class CEvents : public IGameEventListener
{
public:
	void FireGameEvent(CGameEvent* event);
	void PlayerDeath(CGameEvent* event);
	void LocalPlayerDeath(CGameEvent* event);
	void RoundStart(CGameEvent* event);

	bool inited = false;
	bool Intilization();
	bool Destroy();
	//void EmplaceHitData(C_CSPlayerPawn* enity);
};

inline CEvents* Events = new CEvents();
