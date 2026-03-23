#include <windows.h>
#include <iostream>
#include "lua_manager.h"
#include "ipc/ipc_server.h"
#include "game/game_context.h"
#include "hooks/hook_manager.h"
#include <MinHook.h>

LuaManager* g_Lua = nullptr;
IpcServer* g_Ipc = nullptr;

extern "C" __declspec(dllexport) bool HookFunction(void* pTarget, void* pDetour, void** ppOriginal) {
    if (!pTarget || !pDetour) return false;
    __try {
        if (MH_CreateHook(pTarget, pDetour, ppOriginal) != MH_OK) return false;
        return MH_EnableHook(pTarget) == MH_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

extern "C" __declspec(dllexport) bool UnhookFunction(void* pTarget) {
    if (MH_DisableHook(pTarget) != MH_OK) return false;
    return MH_RemoveHook(pTarget) == MH_OK;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    MH_Initialize();

    // Resolve module bases and key game pointers once at attach time.
    GameContext::Get().Init();

    if (GameContext::Get().create_move_addr) {
        HookManager::Get().HookCreateMove(GameContext::Get().create_move_addr);
    }

    g_Lua = new LuaManager();
    if (g_Lua->Init()) {
        g_Ipc = new IpcServer(*g_Lua);
        g_Ipc->Start();
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
            break;
        case DLL_PROCESS_DETACH:
            if (g_Ipc) {
                g_Ipc->Stop();
                delete g_Ipc;
            }
            if (g_Lua) {
                g_Lua->Shutdown();
                delete g_Lua;
            }
            break;
    }
    return TRUE;
}
