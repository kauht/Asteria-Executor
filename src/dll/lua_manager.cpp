#include "lua_manager.h"
#include <vector>
#include <Windows.h>
#include "api/asteria_api.h"
#include "game/game_context.h"
#include "game/inventory_changer.h"

extern LuaManager* g_Lua;

// ---- ConMsg (reads cached fn ptr from GameContext) -------------------------
int lua_ConMsg(lua_State* L) {
    auto& ctx = GameContext::Get();
    if (!ctx.ConMsg) return 0;
    int n = lua_gettop(L);
    std::string msg;
    for (int i = 1; i <= n; i++) {
        if (i > 1) msg += "\t";
        const char* s = lua_tostring(L, i);
        if (s) msg += s;
    }
    msg += "\n";
    ctx.ConMsg("%s", msg.c_str());
    return 0;
}

// ---- Cached context accessors exposed to Lua --------------------------------
int lua_GetEntityList(lua_State* L) {
    lua_pushnumber(L, (double)GameContext::Get().entity_list);
    return 1;
}

int lua_GetLocalController(lua_State* L) {
    lua_pushnumber(L, (double)GameContext::Get().local_controller);
    return 1;
}

int lua_GetClientBase(lua_State* L) {
    lua_pushnumber(L, (double)GameContext::Get().client_base);
    return 1;
}

int lua_GetViewAnglesPtr(lua_State* L) {
    lua_pushnumber(L, (double)GameContext::Get().view_angles_ptr);
    return 1;
}

int lua_ContextReady(lua_State* L) {
    lua_pushboolean(L, GameContext::Get().ready ? 1 : 0);
    return 1;
}

static int lua_GetEconItemCacheCount(lua_State* L) {
    int count = InventoryChanger::GetEconItemCacheCount();
    lua_pushinteger(L, count);
    return 1;
}
// ----------------------------------------------------------------------------

#include "game/skin_changer.h"

int lua_Print(lua_State* L) {
    if (lua_gettop(L) >= 1) {
        std::string txt = lua_tostring(L, 1);
        if (GameContext::Get().ConMsg) {
            GameContext::Get().ConMsg("%s", txt.c_str());
        }
    }
    return 0;
}

int lua_SetSkin(lua_State* L) {
    if (lua_gettop(L) < 5) return 0;
    
    int defIndex = lua_tointeger(L, 1);
    
    SkinChanger::SkinConfig cfg;
    cfg.paintKit = lua_tointeger(L, 2);
    cfg.wear = (float)lua_tonumber(L, 3);
    cfg.seed = lua_tointeger(L, 4);
    cfg.statTrak = lua_tointeger(L, 5);
    cfg.enabled = true;
    
    std::lock_guard<std::mutex> lock(SkinChanger::configMutex);
    SkinChanger::weaponSkins[defIndex] = cfg;
    SkinChanger::forceUpdate = true;
    
    return 0;
}

int lua_SetKnife(lua_State* L) {
    if (lua_gettop(L) < 4) return 0;
    
    SkinChanger::KnifeConfig cfg;
    cfg.defIndex = lua_tointeger(L, 1);
    cfg.paintKit = lua_tointeger(L, 2);
    cfg.wear = (float)lua_tonumber(L, 3);
    cfg.seed = lua_tointeger(L, 4);
    cfg.enabled = true;
    
    std::lock_guard<std::mutex> lock(SkinChanger::configMutex);
    SkinChanger::knifeSkin = cfg;
    SkinChanger::forceUpdate = true;
    
    return 0;
}

int lua_SetGlove(lua_State* L) {
    if (lua_gettop(L) < 4) return 0;
    
    SkinChanger::GloveConfig cfg;
    cfg.defIndex = lua_tointeger(L, 1);
    cfg.paintKit = lua_tointeger(L, 2);
    cfg.wear = (float)lua_tonumber(L, 3);
    cfg.seed = lua_tointeger(L, 4);
    cfg.enabled = true;
    
    std::lock_guard<std::mutex> lock(SkinChanger::configMutex);
    SkinChanger::gloveSkin = cfg;
    SkinChanger::forceUpdate = true;
    
    return 0;
}

int lua_AddInventoryItem(lua_State* L) {
    if (lua_gettop(L) < 1) return 0;
    
    int defIndex = lua_tointeger(L, 1);
    
    InventoryChanger::Initialize();
    
    CEconItem* pItem = InventoryChanger::CreateItem();
    if (pItem) {
        // CEconItem is 72 bytes in CS2. DefIndex is at offset 48.
        uint16_t* pDefIndex = (uint16_t*)((uintptr_t)pItem + 48);
        *pDefIndex = (uint16_t)defIndex;
        // ItemID is at offset 16.
        uint64_t* pID = (uint64_t*)((uintptr_t)pItem + 16);
        *pID = 14000000000ULL + (rand() % 10000); // Base ID
        
        InventoryChanger::AddItemToInventory(pItem);
    }
    
    return 0;
}

int lua_GetInventoryCount(lua_State* L) {
    InventoryChanger::Initialize();
    lua_pushinteger(L, InventoryChanger::GetInventoryCount());
    return 1;
}

int lua_GetLastErrorCode(lua_State* L) {
    lua_pushinteger(L, InventoryChanger::GetLastErrorCode());
    return 1;
}

int lua_GetRawWord(lua_State* L) {
    int idx = (int)lua_tointeger(L, 1);
    lua_pushinteger(L, InventoryChanger::GetRawWord(idx));
    return 1;
}

int lua_GetEconItemRawWord(lua_State* L) {
    int itemIndex = (int)lua_tointeger(L, 1);
    int wordIndex = (int)lua_tointeger(L, 2);
    lua_pushinteger(L, InventoryChanger::GetEconItemRawWord(itemIndex, wordIndex));
    return 1;
}

int lua_ScanInventoryManager(lua_State* L) {
    InventoryChanger::ScanInventoryManager();
    return 0;
}

int lua_GetScanResultCount(lua_State* L) {
    lua_pushinteger(L, InventoryChanger::GetScanResultCount());
    return 1;
}

int lua_GetScanResult(lua_State* L) {
    int idx = (int)lua_tointeger(L, 1);
    lua_pushstring(L, InventoryChanger::GetScanResult(idx).c_str());
    return 1;
}

int lua_SendLog(lua_State* L) {
    int n = lua_gettop(L);
    std::string str = "";
    for (int i = 1; i <= n; i++) {
        if (i > 1) str += "\t";
        const char* s = lua_tostring(L, i);
        if (s) str += s;
    }
    if (g_Lua) {
        g_Lua->GetLogClient().SendLog(str);
    }
    return 0;
}

#include "hooks/hook_manager.h"

int lua_RegisterCreateMoveCallback(lua_State* L) {
    if (lua_isfunction(L, 1)) {
        lua_pushvalue(L, 1); // Duplicate function to top of stack
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        HookManager::Get().SetCreateMoveCallback(ref);
        lua_pushboolean(L, true);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

int lua_UnhookCreateMove(lua_State* L) {
    HookManager::Get().UnhookCreateMove();
    lua_pushboolean(L, true);
    return 1;
}

int lua_GetCurrentUserCmd(lua_State* L) {
    if (lua_gettop(L) < 1) {
        lua_pushnil(L);
        return 1;
    }
    void* controller = nullptr;
    if (lua_isnumber(L, 1)) {
        controller = (void*)(uintptr_t)lua_tonumber(L, 1);
    } else if (lua_islightuserdata(L, 1)) {
        controller = lua_touserdata(L, 1);
    }

    if (!controller) {
        lua_pushnil(L);
        return 1;
    }

    void* cmd = GameContext::Get().GetCurrentUserCmd(controller);
    if (cmd) {
        lua_pushlightuserdata(L, cmd);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

extern "C" __declspec(dllexport) void* CallFn(void* addr, int argCount, uint64_t* args) {
    if (!addr) return nullptr;
    void* ret = nullptr;
    
    switch(argCount) {
         case 0: ret = ((void*(*)())addr)(); break;
         case 1: ret = ((void*(*)(void*))addr)((void*)args[0]); break;
         case 2: ret = ((void*(*)(void*, void*))addr)((void*)args[0], (void*)args[1]); break;
         case 3: ret = ((void*(*)(void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2]); break;
         case 4: ret = ((void*(*)(void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3]); break;
         case 5: ret = ((void*(*)(void*, void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3], (void*)args[4]); break;
         case 6: ret = ((void*(*)(void*, void*, void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3], (void*)args[4], (void*)args[5]); break;
         case 7: ret = ((void*(*)(void*, void*, void*, void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3], (void*)args[4], (void*)args[5], (void*)args[6]); break;
         case 8: ret = ((void*(*)(void*, void*, void*, void*, void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3], (void*)args[4], (void*)args[5], (void*)args[6], (void*)args[7]); break;
         case 9: ret = ((void*(*)(void*, void*, void*, void*, void*, void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3], (void*)args[4], (void*)args[5], (void*)args[6], (void*)args[7], (void*)args[8]); break;
         case 10: ret = ((void*(*)(void*, void*, void*, void*, void*, void*, void*, void*, void*, void*))addr)((void*)args[0], (void*)args[1], (void*)args[2], (void*)args[3], (void*)args[4], (void*)args[5], (void*)args[6], (void*)args[7], (void*)args[8], (void*)args[9]); break;
         default: break; 
    }
    return ret;
}

LuaManager::LuaManager() : L(nullptr) {}

LuaManager::~LuaManager() {
    Shutdown();
}

bool LuaManager::Init() {
    L = luaL_newstate();
    if (!L) return false;

    luaL_openlibs(L); 
    
    lua_register(L, "SendLogCpp", lua_SendLog);
    lua_register(L, "ConMsgCpp", lua_ConMsg);
    lua_register(L, "RegisterCreateMoveCallbackCpp", lua_RegisterCreateMoveCallback);
    lua_register(L, "UnhookCreateMoveCpp", lua_UnhookCreateMove);
    lua_register(L, "GetEntityListCpp", lua_GetEntityList);
    lua_register(L, "GetLocalControllerCpp", lua_GetLocalController);
    lua_register(L, "GetClientBaseCpp", lua_GetClientBase);
    lua_register(L, "GetViewAnglesPtrCpp", lua_GetViewAnglesPtr);
    lua_register(L, "ContextReadyCpp", lua_ContextReady);
    lua_register(L, "GetCurrentUserCmdCpp", lua_GetCurrentUserCmd);
    
    lua_register(L, "PrintCpp", lua_Print);
    lua_register(L, "SetSkinCpp", lua_SetSkin);
    lua_register(L, "SetKnifeCpp", lua_SetKnife);
    lua_register(L, "SetGloveCpp", lua_SetGlove);
    lua_register(L, "AddInventoryItemCpp", lua_AddInventoryItem);
    lua_register(L, "GetInventoryCountCpp", lua_GetInventoryCount);
    lua_register(L, "GetLastErrorCodeCpp", lua_GetLastErrorCode);
    lua_register(L, "GetRawWordCpp", lua_GetRawWord);
    lua_register(L, "GetEconItemRawWordCpp", lua_GetEconItemRawWord);
    lua_register(L, "GetEconItemCacheCountCpp", lua_GetEconItemCacheCount);
    lua_register(L, "ScanInventoryManagerCpp", lua_ScanInventoryManager);
    lua_register(L, "GetScanResultCountCpp", lua_GetScanResultCount);
    lua_register(L, "GetScanResultCpp", lua_GetScanResult);
    
    // Pass CallFn address to Lua
    lua_pushlightuserdata(L, (void*)&CallFn);
    lua_setglobal(L, "CallFnNativePtr");


    const char* overridePrint = 
        "local old_print = print\n"
        "print = function(...)\n"
        "    local str = \"\"\n"
        "    local n = select('#', ...)\n"
        "    for i=1, n do\n"
        "        str = str .. tostring(select(i, ...))\n"
        "        if i < n then str = str .. \"\\t\" end\n"
        "    end\n"
        "    SendLogCpp(str)\n"
        "end\n";

    luaL_dostring(L, overridePrint);


    if (luaL_dostring(L, asteriaApi) != LUA_OK) {
        std::string err = "API Load Error: ";
        const char* s = lua_tostring(L, -1);
        if (s) err += s;
        GetLogClient().SendLog(err);
        lua_pop(L, 1);
        return false;
    }

    return true;
}

void LuaManager::Shutdown() {
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

bool LuaManager::ExecuteString(const std::string& code) {
    if (!L) return false;

    int r = luaL_dostring(L, code.c_str());
    if (r != LUA_OK) {
        std::string err = "Lua Error: ";
        const char* s = lua_tostring(L, -1);
        if (s) err += s;
        GetLogClient().SendLog(err);
        lua_pop(L, 1);
        return false;
    }
    return true;
}
