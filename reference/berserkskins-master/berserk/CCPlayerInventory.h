#pragma once
#include "memory.h"
#include "utlvector.h"
class CEconItem;

class CCSPlayerInventory;
class CGCClientSharedObjectCache;
struct SOID_t {
    uint64_t m_id;
    uint32_t m_type;
    uint32_t m_padding;
};

class CGCClient {
public:
    CGCClientSharedObjectCache* FindSOCache(SOID_t ID,
        bool bCreateIfMissing = true);
};

class CGCClientSystem {
public:
    static CGCClientSystem* GetInstance();

    CGCClient* GetCGCClient() {
        return reinterpret_cast<CGCClient*>((uintptr_t)(this) + 0xB8);
    }
};


class CCSInventoryManager {
public:
    static CCSInventoryManager* GetInstance();

    auto EquipItemInLoadout(int iTeam, int iSlot, uint64_t iItemID) {
        return M::call_virtual<bool>(this, 66, iTeam, iSlot, iItemID);

    }

    auto GetLocalInventory() {
        return M::call_virtual<CCSPlayerInventory*>(this, 69);

    }
};
class CSharedObject;

class CGCClientSharedObjectTypeCache {
public:
    auto AddObject(CSharedObject* pObject) {
        return M::call_virtual<bool>(this, 1, pObject);
    }

    auto RemoveObject(CSharedObject* soIndex) {
        return M::call_virtual<CSharedObject*>(this, 3, soIndex);

    }

    template <typename T>
    auto& GetVecObjects() {
        return *reinterpret_cast<CUtlVector<T>*>((uintptr_t)(this) + 0x8);
    }
};

enum ESOCacheEvent {
    /// Dummy sentinel value
    eSOCacheEvent_None = 0,

    /// We received a our first update from the GC and are subscribed
    eSOCacheEvent_Subscribed = 1,

    /// We lost connection to GC or GC notified us that we are no longer
    /// subscribed. Objects stay in the cache, but we no longer receive updates
    eSOCacheEvent_Unsubscribed = 2,

    /// We received a full update from the GC on a cache for which we were
    /// already subscribed. This can happen if connectivity is lost, and then
    /// restored before we realized it was lost.
    eSOCacheEvent_Resubscribed = 3,

    /// We received an incremental update from the GC about specific object(s)
    /// being added, updated, or removed from the cache
    eSOCacheEvent_Incremental = 4,

    /// A lister was added to the cache
    /// @see CGCClientSharedObjectCache::AddListener
    eSOCacheEvent_ListenerAdded = 5,

    /// A lister was removed from the cache
    /// @see CGCClientSharedObjectCache::RemoveListener
    eSOCacheEvent_ListenerRemoved = 6,
};
class C_EconItemView;
class CCSPlayerInventory {
public:
    static CCSPlayerInventory* GetInstance();
    auto SOCreated(SOID_t owner, CSharedObject* pObject, ESOCacheEvent eEvent)
    {
        return M::call_virtual<void>(this, 0, owner, pObject, eEvent);
    }
    //auto SOCreated(SOID_t owner, CSharedObject* pObject, ESOCacheEvent eEvent) {
    //    return CALL_VIRTUAL(void, 0, this, owner, pObject, eEvent);
    //}

    //auto SOUpdated(SOID_t owner, CSharedObject* pObject, ESOCacheEvent eEvent) {
    //    return CALL_VIRTUAL(void, 1, this, owner, pObject, eEvent);
    //}

    CEconItem* GetSOCDataForItem(uint64_t itemID);


    auto SODestroyed(SOID_t owner, CSharedObject* pObject,
        ESOCacheEvent eEvent) {
        return M::call_virtual<void>(this, 2, owner, pObject, eEvent);
    }

    //auto GetItemInLoadout(int iClass, int iSlot) {
    //    return CALL_VIRTUAL(C_EconItemView*, 8, this, iClass, iSlot);
    //}
    auto GetItemInLoadout(int iClass, int iSlot) {
        return M::call_virtual<C_EconItemView*>(this, 8, iClass, iSlot);
    }

    bool AddEconItem(CEconItem* pItem);

    //bool AddEconItem(CEconItem* pItem);
    void RemoveEconItem(CEconItem* pItem);
    std::pair<uint64_t, uint32_t> GetHighestIDs();


    auto GetOwner() {
        return *reinterpret_cast<SOID_t*>((uintptr_t)(this) + 0x10);
    }
    CGCClientSharedObjectCache* GetSOCache() {
        return *reinterpret_cast<CGCClientSharedObjectCache**>(reinterpret_cast<uint8_t*>(this) + 0x68);
    }
};





class CGCClientSharedObjectCache {
public:
    CGCClientSharedObjectTypeCache* CreateBaseTypeCache(int nClassID);
};

