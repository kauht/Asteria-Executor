#include "hook_manager.h"
#include "lua_manager.h"
#include <MinHook.h>
#include <iostream>

extern LuaManager *g_Lua;

// Typedef for safe 6-argument CreateMove which preserves stack frames in x64
typedef bool(__fastcall *CreateMoveFn)(void *, void *, void *, void *, void *,
                                       void *);

#include "../game/skin_changer.h"

bool __fastcall Detour_CreateMove(void *rcx, void *rdx, void *r8, void *r9,
                                  void *arg5, void *arg6) {
  SkinChanger::Tick();

  HookManager &hm = HookManager::Get();
  int ref = hm.GetCreateMoveCallback();

  if (ref != -1 && g_Lua) {
    lua_State *L = g_Lua->GetState();
    if (L) {
      // Get function from registry
      lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

      // Push arguments as lightuserdata or numbers for FFI consumption
      lua_pushnumber(L, (double)(uintptr_t)rcx);
      lua_pushnumber(L, (double)(uintptr_t)rdx);
      lua_pushnumber(L, (double)(uintptr_t)r8);
      lua_pushnumber(L, (double)(uintptr_t)r9);

      // Call function with 4 args, 0 results
      if (lua_pcall(L, 4, 0, 0) != 0) {
        const char *err = lua_tostring(L, -1);
        if (err) {
          g_Lua->GetLogClient().SendLog(
              std::string("CreateMove Callback Error: ") + err);
        }
        lua_pop(L, 1);
      }
    }
  }

  // Call original
  CreateMoveFn original = (CreateMoveFn)hm.GetOriginalCreateMove();
  if (original) {
    return original(rcx, rdx, r8, r9, arg5, arg6);
  }
  return false;
}

bool HookManager::Init() { return true; }

void HookManager::Shutdown() { UnhookCreateMove(); }

void HookManager::SetCreateMoveCallback(int ref) {
  std::lock_guard<std::mutex> lock(m_Mutex);
  if (m_CreateMoveCallbackRef != -1 && g_Lua) {
    luaL_unref(g_Lua->GetState(), LUA_REGISTRYINDEX, m_CreateMoveCallbackRef);
  }
  m_CreateMoveCallbackRef = ref;
}

bool HookManager::HookCreateMove(void *pTarget) {
  if (!pTarget)
    return false;
  m_CreateMoveTarget = pTarget;

  // Force remove previous hook on this address if it exists
  MH_DisableHook(pTarget);
  MH_RemoveHook(pTarget);

  int status =
      MH_CreateHook(pTarget, &Detour_CreateMove, &m_OriginalCreateMove);
  if (status != MH_OK) {
    if (g_Lua) {
      g_Lua->GetLogClient().SendLog(std::string("MH_CreateHook Failed: ") +
                                    std::to_string(status));
    }
    return false;
  }

  if (MH_EnableHook(pTarget) != MH_OK) {
    return false;
  }

  return true;
}

void HookManager::UnhookCreateMove() {
  if (m_CreateMoveTarget) {
    MH_DisableHook(m_CreateMoveTarget);
    MH_RemoveHook(m_CreateMoveTarget);
    m_CreateMoveTarget = nullptr;
    m_OriginalCreateMove = nullptr;
  }
  if (m_CreateMoveCallbackRef != -1 && g_Lua) {
    luaL_unref(g_Lua->GetState(), LUA_REGISTRYINDEX, m_CreateMoveCallbackRef);
    m_CreateMoveCallbackRef = -1;
  }
}
