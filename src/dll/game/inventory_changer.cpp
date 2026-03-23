#include "inventory_changer.h"
#include "../api/sig_scanner.h"
#include "lua_manager.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <windows.h>

// Steam Object ID Type
struct SOID_t {
  uint64_t m_id;
  uint32_t m_type;
  uint32_t m_padding;
};

// SOCache event
#define eSOCacheEvent_Incremental 4

// Confirmed offsets from hex dump:
// pRaw[i] = uint32 at byte i*4
// Off 08 (byte 32) = 14 = m_nItemCount ← CONFIRMED
// Off 09 (byte 36) = capacity
// Off 10 (byte 40) = m_pItems ptr low
#define OFF_INV_ITEM_COUNT 32
#define OFF_INV_SO_CACHE 0x68
#define OFF_INV_OWNER 0x10
#define OFF_CEconItem_ID 16
#define OFF_CEconItem_AccountID 40
#define OFF_CEconItem_Inventory 44
#define OFF_CEconItem_DefIndex 48

typedef CEconItem *(__cdecl *FnCreateInstance)();
typedef void *(__thiscall *FnCreateBaseTypeCache)(void *, int);

static FnCreateInstance oCreateInstance = nullptr;
static uintptr_t getInventoryManagerAddr = 0;
static FnCreateBaseTypeCache oCreateBaseTypeCache = nullptr;
static int lastErrorCode = 0;
static std::vector<std::string> scanResults;

// ============================================================================
// All SEH blocks MUST be in plain functions with no C++ objects on the stack
// ============================================================================

static void *safe_read_ptr(uintptr_t addr) {
  __try {
    return *reinterpret_cast<void **>(addr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

static int safe_read_int(uintptr_t addr) {
  __try {
    return *reinterpret_cast<int *>(addr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return -1;
  }
}

static void safe_write_u32(uintptr_t addr, uint32_t val) {
  __try {
    *reinterpret_cast<uint32_t *>(addr) = val;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static void safe_write_u64(uintptr_t addr, uint64_t val) {
  __try {
    *reinterpret_cast<uint64_t *>(addr) = val;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static void safe_write_u16(uintptr_t addr, uint16_t val) {
  __try {
    *reinterpret_cast<uint16_t *>(addr) = val;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static uint32_t safe_read_u32(uintptr_t addr) {
  __try {
    return *reinterpret_cast<uint32_t *>(addr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
}

// VFunc caller (no C++ objects, SEH safe)
static void *call_vfunc_ptr(void *pThis, int idx) {
  __try {
    void **vt = *reinterpret_cast<void ***>(pThis);
    typedef void *(__fastcall * Fn)(void *);
    return reinterpret_cast<Fn>(vt[idx])(pThis);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

// For calling AddObject(pItem) — 2 args
static bool call_addobject(void *pTypeCache, void *pItem) {
  __try {
    void **vt = *reinterpret_cast<void ***>(pTypeCache);
    typedef bool(__fastcall * Fn)(void *, void *);
    return reinterpret_cast<Fn>(vt[1])(pTypeCache, pItem);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

// For calling SOCreated(owner, pItem, event) on CCSPlayerInventory
static void call_socreated(void *pInv, SOID_t owner, void *pItem, int evt) {
  __try {
    void **vt = *reinterpret_cast<void ***>(pInv);
    typedef void(__fastcall * Fn)(void *, SOID_t, void *, int);
    reinterpret_cast<Fn>(vt[0])(pInv, owner, pItem, evt);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

// CreateBaseTypeCache call
static void *call_create_type_cache(FnCreateBaseTypeCache fn, void *socache,
                                    int classID) {
  __try {
    return fn(socache, classID);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

// GetInventoryManager call
static void *call_get_mgr(uintptr_t addr) {
  __try {
    typedef void *(__fastcall * Fn)();
    return reinterpret_cast<Fn>(addr)();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

// CreateInstance call
static CEconItem *call_create_instance(FnCreateInstance fn) {
  __try {
    return fn();
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

static bool call_socreated_ptr(void *pInv, SOID_t *pOwner, void *pItem,
                               int evt) {
  __try {
    void **vt = *reinterpret_cast<void ***>(pInv);
    typedef void(__fastcall * FnSOCreated)(void *, SOID_t *, void *, int);
    reinterpret_cast<FnSOCreated>(vt[0])(pInv, pOwner, pItem, evt);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

// ============================================================================
// Public API
// ============================================================================

bool InventoryChanger::Initialize() {
  if (!oCreateInstance)
    oCreateInstance = (FnCreateInstance)ScanPattern(
        "client.dll", "48 83 EC 28 B9 48 00 00 00 E8 ?? ?? ?? ?? 48 85");

  if (!getInventoryManagerAddr)
    getInventoryManagerAddr = (uintptr_t)ScanPattern(
        "client.dll",
        "48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 8B 91 ?? ?? ?? ?? B8");

  if (!oCreateBaseTypeCache)
    oCreateBaseTypeCache = (FnCreateBaseTypeCache)ScanPattern(
        "client.dll", "40 53 48 83 EC ?? 4C 8B 49 ?? 44 8B D2");

  return oCreateInstance != nullptr && getInventoryManagerAddr != 0;
}

CEconItem *InventoryChanger::CreateItem() {
  if (!oCreateInstance) {
    lastErrorCode = 10;
    return nullptr;
  }
  CEconItem *p = call_create_instance(oCreateInstance);
  if (!p) {
    lastErrorCode = 11;
  }
  return p;
}

bool InventoryChanger::AddItemToInventory(CEconItem *pItem) {
  if (!pItem) {
    lastErrorCode = 1;
    return false;
  }

  // 1. Get manager
  void *pMgr = call_get_mgr(getInventoryManagerAddr);
  if (!pMgr) {
    lastErrorCode = 2;
    return false;
  }

  // 2. Get local inventory (VFunc 70)
  void *pInv = call_vfunc_ptr(pMgr, 70);
  if (!pInv) {
    lastErrorCode = 3;
    return false;
  }

  // 3. Set item metadata
  SOID_t *pOwner = reinterpret_cast<SOID_t *>((uintptr_t)pInv + OFF_INV_OWNER);
  // AccountID = lower 32 bits of SOID_t::m_id (it's a SteamID)
  uint64_t ownerID = *reinterpret_cast<uint64_t *>((uintptr_t)pOwner);
  uint32_t accountID = (uint32_t)(ownerID & 0xFFFFFFFF);

  int nCount = safe_read_int((uintptr_t)pInv + OFF_INV_ITEM_COUNT);
  if (nCount < 0)
    nCount = 0;

  // Write the validated CEconItem fields based on true CS2 72-byte layout
  safe_write_u64((uintptr_t)pItem + OFF_CEconItem_ID,
                 14000000000ULL + (uint64_t)nCount + 1);
  safe_write_u32((uintptr_t)pItem + OFF_CEconItem_AccountID, accountID);
  safe_write_u32((uintptr_t)pItem + OFF_CEconItem_Inventory,
                 3000 + (uint32_t)nCount + 1);
  // DefIndex is already set by lua_manager.cpp at offset 48.

  // NOTE: We do NOT write to byte 444 (unknown field). The def index at 442
  // is set by lua_AddInventoryItem before calling us. Leave other fields alone.

  // 4. Get SOCache at +0x68
  void *pSOCache = safe_read_ptr((uintptr_t)pInv + OFF_INV_SO_CACHE);
  if (!pSOCache) {
    lastErrorCode = 6;
    return false;
  }

  // 5. Get/Create SOTypeCache for class 1 (CEconItem)
  void *pTypeCache = nullptr;
  if (oCreateBaseTypeCache)
    pTypeCache = call_create_type_cache(oCreateBaseTypeCache, pSOCache, 1);
  if (!pTypeCache) {
    lastErrorCode = 7;
    return false;
  }

  // 6. AddObject (VFunc 1)
  bool bAdded = call_addobject(pTypeCache, pItem);
  if (!bAdded) {
    lastErrorCode = 8;
    return false;
  }

  // 7. Notify game via SOCreated (VFunc 0 of CCSPlayerInventory)
  // MSVC x64 ABI: SOID_t (16 bytes) is passed via hidden pointer, so we call
  // with signature: Fn(this, SOID_t*, obj)
  SOID_t ownerVal = {};
  __try {
    ownerVal = *pOwner;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }

  typedef void(__fastcall * FnSOCreated)(void *, SOID_t *, void *);
  FnSOCreated oSOCreated =
      (FnSOCreated)safe_read_ptr((uintptr_t)safe_read_ptr((uintptr_t)pInv));
  if (oSOCreated) {
    __try {
      oSOCreated(pInv, &ownerVal, pItem);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      lastErrorCode = 13;
      return false;
    }
  } else {
    lastErrorCode = 13;
    return false;
  }

  lastErrorCode = 0;
  return true;
}

int InventoryChanger::GetInventoryCount() {
  if (!getInventoryManagerAddr)
    return -1;
  void *pMgr = call_get_mgr(getInventoryManagerAddr);
  if (!pMgr)
    return -2;
  void *pInv = call_vfunc_ptr(pMgr, 70);
  if (!pInv)
    return -3;
  return safe_read_int((uintptr_t)pInv + OFF_INV_ITEM_COUNT);
}

int InventoryChanger::GetRawWord(int index) {
  if (!getInventoryManagerAddr)
    return -1;
  void *pMgr = call_get_mgr(getInventoryManagerAddr);
  if (!pMgr)
    return -2;
  void *pInv = call_vfunc_ptr(pMgr, 70);
  if (!pInv)
    return -3;
  return (int)safe_read_u32((uintptr_t)pInv + (uintptr_t)(index * 4));
}

int InventoryChanger::GetEconItemRawWord(int index, int wordIndex) {
  if (!getInventoryManagerAddr)
    return -1;
  void *pMgr = call_get_mgr(getInventoryManagerAddr);
  if (!pMgr)
    return -2;
  void *pInv = call_vfunc_ptr(pMgr, 70); // VFunc 70 returns CCSPlayerInventory
  if (!pInv)
    return -3;

  void *pSOCache = safe_read_ptr((uintptr_t)pInv + OFF_INV_SO_CACHE);
  if (!pSOCache)
    return -4;

  void *pTypeCache = nullptr;
  if (oCreateBaseTypeCache)
    pTypeCache = call_create_type_cache(oCreateBaseTypeCache, pSOCache, 1);
  if (!pTypeCache)
    return -5;

  int size = safe_read_int((uintptr_t)pTypeCache + 8);
  uintptr_t elements = (uintptr_t)safe_read_ptr((uintptr_t)pTypeCache + 16);
  if (!elements)
    return -7; // Adjusted error code

  if (index < 0 || index >= size)
    return -6;

  uintptr_t pItem = (uintptr_t)safe_read_ptr(
      elements + index * 8); // CUtlVector stores pointers (8 bytes each)
  if (!pItem)
    return -7;

  if (wordIndex < 0 || wordIndex > 17)
    return -8; // CEconItem is 72 bytes max (18 words * 4 bytes/word)

  return safe_read_int(pItem + wordIndex * 4);
}

int InventoryChanger::GetEconItemCacheCount() {
  if (!getInventoryManagerAddr)
    return -1;
  void *pMgr = call_get_mgr(getInventoryManagerAddr);
  if (!pMgr)
    return -2;
  void *pInv = call_vfunc_ptr(pMgr, 70);
  if (!pInv)
    return -3;

  void *pSOCache = safe_read_ptr((uintptr_t)pInv + OFF_INV_SO_CACHE);
  if (!pSOCache)
    return -4;

  void *pTypeCache = nullptr;
  if (oCreateBaseTypeCache)
    pTypeCache = call_create_type_cache(oCreateBaseTypeCache, pSOCache, 1);
  if (!pTypeCache)
    return 0;

  int size = safe_read_u32((uintptr_t)pTypeCache + 8);
  uintptr_t elements = (uintptr_t)safe_read_ptr((uintptr_t)pTypeCache + 16);

  // Debugging print
  char buf[256];
  sprintf_s(buf,
            "[MyCheat] pInv: %p | SOCache: %p | TypeCache: %p | Size: %d | "
            "Elements: %p",
            pInv, pSOCache, pTypeCache, size, (void *)elements);

  extern LuaManager *g_Lua;
  if (g_Lua)
    g_Lua->GetLogClient().SendLog(buf);

  return size;
}

int InventoryChanger::GetLastErrorCode() { return lastErrorCode; }

void InventoryChanger::ScanInventoryManager() {
  Initialize();
  scanResults.clear();
  scanResults.push_back("Scanning CCSInventoryManager Singleton Structure:");

  if (!getInventoryManagerAddr) {
    scanResults.push_back("Manager addr not found");
    return;
  }
  void *pMgr = call_get_mgr(getInventoryManagerAddr);
  for (int r = 0; r < 20 && !pMgr; ++r) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pMgr = call_get_mgr(getInventoryManagerAddr);
  }
  if (!pMgr) {
    scanResults.push_back("Manager Null After Retries");
    return;
  }

  char buf[128];
  void *pVT = safe_read_ptr((uintptr_t)pMgr);
  snprintf(buf, sizeof(buf), "Base VTable address: %p", pVT);
  scanResults.push_back(buf);

  void *pInv = call_vfunc_ptr(pMgr, 70);
  snprintf(buf, sizeof(buf), "Offset 259392 (LocalInventory): %p", pInv);
  scanResults.push_back(buf);

  if (pInv) {
    void *pSOCache = safe_read_ptr((uintptr_t)pInv + OFF_INV_SO_CACHE);
    snprintf(buf, sizeof(buf), "SOCache (@+0x68): %p", pSOCache);
    scanResults.push_back(buf);

    snprintf(buf, sizeof(buf), "CreateBaseTypeCache fn: %p",
             (void *)oCreateBaseTypeCache);
    scanResults.push_back(buf);
  }
}

int InventoryChanger::GetScanResultCount() { return (int)scanResults.size(); }

std::string InventoryChanger::GetScanResult(int index) {
  if (index < 0 || index >= (int)scanResults.size())
    return "";
  return scanResults[index];
}
