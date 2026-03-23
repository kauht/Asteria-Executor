#include "CCPlayerInventory.h"
#include <utility>
#include "i_client.h"
#include "memory.h"
#include "utlvector.h"
CCSInventoryManager* CCSInventoryManager::GetInstance() {
    using FnGetInstanceS = CCSInventoryManager * (__fastcall*)();
    static FnGetInstanceS oGetInstanceS = reinterpret_cast<FnGetInstanceS>(M::FindPattern(L"client.dll", "48 8d 05 ?? ?? ?? ?? c3 cc cc cc cc cc cc cc cc 8b 91 ?? ?? ?? ?? b8"));

    return oGetInstanceS();
}


CGCClientSharedObjectCache* CGCClient::FindSOCache(SOID_t ID,
    bool bCreateIfMissing) {
    using FnFindSOCache = CGCClientSharedObjectCache * (__fastcall*)(CGCClient*, SOID_t, bool);

    static FnFindSOCache oFindSOCache = reinterpret_cast<FnFindSOCache>(
        M::ResolveRelativeAddress(
            reinterpret_cast<uint8_t*>(M::FindPattern(L"client.dll", "E8 ? ? ? ? 48 8B F0 48 85 C0 74 0E 4C 8B C3"
            )),
            0x1,
            0x0
        )
        );

    return oFindSOCache(this, ID, bCreateIfMissing);
}


CGCClientSharedObjectTypeCache* CGCClientSharedObjectCache::CreateBaseTypeCache(int nClassID)
{
    using fnCreateBaseTypeCache = CGCClientSharedObjectTypeCache * (__thiscall*)(void*, int);
    static fnCreateBaseTypeCache oCreateBaseTypeCache = reinterpret_cast<fnCreateBaseTypeCache>( M::FindPattern(L"client.dll", "40 53 48 83 EC ? 4C 8B 49 ? 44 8B D2"));

    if (!oCreateBaseTypeCache)
        return nullptr;
    return oCreateBaseTypeCache(this, nClassID);

}

CGCClientSystem* CGCClientSystem::GetInstance() {
    using FnGetInstanceSs = CGCClientSystem * (__fastcall*)();

    static FnGetInstanceSs fnGetClientSystem =
        reinterpret_cast<FnGetInstanceSs>(
            M::ResolveRelativeAddress(
                M::FindPattern(L"client.dll", "E8 ? ? ? ? 48 8B 4F 10 8B 1D ? ? ? ?"),
                0x1, 0x0
            )
            );

    if (!fnGetClientSystem) {
        printf("[E] Failed to resolve CGCClientSystem::GetInstance\n");
        return nullptr;
    }

    return fnGetClientSystem();
}
CCSPlayerInventory* CCSPlayerInventory::GetInstance() {
    CCSInventoryManager* pInventoryManager = CCSInventoryManager::GetInstance();
    if (!pInventoryManager) return nullptr;

    return pInventoryManager->GetLocalInventory();
}

CGCClientSharedObjectTypeCache* CreateBaseTypeCache(
    CCSPlayerInventory* pInventory) {

    CGCClientSharedObjectCache* SOCache = pInventory->GetSOCache(); // offset 0x68
    if (!SOCache) return nullptr;

    return SOCache->CreateBaseTypeCache(1);
}

bool CCSPlayerInventory::AddEconItem(CEconItem* pItem) {
    // Helper function to aid in adding items.
    if (!pItem) return false;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (!pSOTypeCache || !pSOTypeCache->AddObject((CSharedObject*)pItem))
        return false;

    SOCreated(GetOwner(), (CSharedObject*)pItem, eSOCacheEvent_Incremental);
    return true;
}
//
CEconItem* CCSPlayerInventory::GetSOCDataForItem(uint64_t itemID) {
    CEconItem* pSOCData = nullptr;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (pSOTypeCache) {
        const CUtlVector<CEconItem*>& vecItems =
            pSOTypeCache->GetVecObjects<CEconItem*>();

        for (CEconItem* it : vecItems) {
            if (it && it->m_ulID == itemID) {
                pSOCData = it;
                break;
            }
        }
    }

    return pSOCData;
}

void CCSPlayerInventory::RemoveEconItem(CEconItem* pItem) {
    // Helper function to aid in removing items.
    if (!pItem) return;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (!pSOTypeCache) return;

    const CUtlVector<CEconItem*>& pSharedObjects =
        pSOTypeCache->GetVecObjects<CEconItem*>();
    if (!pSharedObjects.Exists(pItem)) return;

    SODestroyed(GetOwner(), (CSharedObject*)pItem, eSOCacheEvent_Incremental);
    pSOTypeCache->RemoveObject((CSharedObject*)pItem);

    pItem->Destruct();
}

std::pair<uint64_t, uint32_t> CCSPlayerInventory::GetHighestIDs() {
    uint64_t maxItemID = 0;
    uint32_t maxInventoryID = 0;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (pSOTypeCache) {
        const CUtlVector<CEconItem*>& vecItems =
            pSOTypeCache->GetVecObjects<CEconItem*>();

        for (CEconItem* it : vecItems) {
            if (!it) continue;

            // Checks if item is default.
            if ((it->m_ulID & 0xF000000000000000) != 0) continue;

            maxItemID = std::max(maxItemID, it->m_ulID);
            maxInventoryID = std::max(maxInventoryID, it->m_unInventory);
        }
    }

    return std::make_pair(maxItemID, maxInventoryID);
}
