#include "game_context.h"
#include "../api/sig_scanner.h"
#include "../lua_manager.h"
#include <stdio.h>
#include <Windows.h>

// Resolve a module base, waiting up to `timeout_ms` for it to appear.
static uintptr_t WaitForModule(const char* name, DWORD timeout_ms = 5000) {
    DWORD elapsed = 0;
    while (elapsed < timeout_ms) {
        HMODULE h = GetModuleHandleA(name);
        if (h) return (uintptr_t)h;
        Sleep(100);
        elapsed += 100;
    }
    return 0;
}

// Safe 8-byte pointer read (returns 0 on failure).
static uintptr_t SafeReadPtr(uintptr_t addr) {
    if (!addr) return 0;
    __try {
        return *(uintptr_t*)addr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

bool GameContext::Init() {
    // ── Modules ─────────────────────────────────────────────────────────────
    client_base = WaitForModule("client.dll");
    engine_base = WaitForModule("engine2.dll");
    tier0_base  = WaitForModule("tier0.dll");

    if (!client_base || !tier0_base) return false;

    // ── Key pointers (offsets from cs2-dumper 2026-03-19) ───────────────────
    // dwEntityList            0x24AF268  (client.dll)
    entity_list      = SafeReadPtr(client_base + 0x24AF268);

    // dwLocalPlayerController 0x22F4188  (client.dll)
    // This is a pointer; dereference once to get the actual address.
    local_controller = SafeReadPtr(client_base + 0x22F4188);

    // dwViewAngles            0x231A648  (client.dll)
    view_angles_ptr  = client_base + 0x231A648;

    // CreateMove Pattern Scan
    create_move_addr = ScanPattern("client.dll", "48 89 5C 24 ?? 55 57 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 8B 01 48 8B F9");

    // Resolve CCSGOInput Helpers
    uintptr_t input_addr = (uintptr_t)ScanPattern("client.dll", "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B CF");
    if (input_addr) {
        int offset = *(int*)(input_addr + 3);
        pCCSGOInput = *(void**)(input_addr + 7 + offset);
    }
    
    // Robust longer signatures for accurate resolution
    GetCUserCmdTick = (FnGetCUserCmdTick)ScanPattern("client.dll", "48 83 EC ? 4C 8B 0D ? ? ? ? 48 8B F9 8B 0A 33 DB 4D 8B F0");
    GetCUserCmdArray = (FnGetCUserCmdArray)ScanPattern("client.dll", "48 89 4C 24 ? 41 56 41 57 48 83 EC ? 4C 63 FA 4C 8B F1");
    GetCUserCmdBySequenceNumber = (FnGetCUserCmdBySequenceNumber)ScanPattern("client.dll", "40 53 48 83 EC ? 8B 0D ? ? ? ? 48 8D 05 ? ? ? ? 48 89 05");

    // Verbose pointer logging
    extern LuaManager* g_Lua;
    if (g_Lua) {
        char buf[512];
        sprintf_s(buf, "[Init] pCCSGOInput: %p | GetCUserCmdTick: %p | GetCUserCmdArray: %p | GetCUserCmdBySequenceNumber: %p", 
            pCCSGOInput, GetCUserCmdTick, GetCUserCmdArray, GetCUserCmdBySequenceNumber);
        g_Lua->GetLogClient().SendLog(std::string(buf));
    }

    // ── ConMsg ───────────────────────────────────────────────────────────────
    ConMsg = (ConMsgFn)GetProcAddress((HMODULE)tier0_base, "?ConMsg@@YAXPEBDZZ");

    ready = (entity_list != 0 && pCCSGOInput != nullptr);
    return ready;
}

void* GameContext::GetCurrentUserCmd(void* controller) {
    extern LuaManager* g_Lua;
    char buf[128];

    if (!pCCSGOInput || !GetCUserCmdTick || !GetCUserCmdArray || !GetCUserCmdBySequenceNumber || !controller) {
        if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Missing pointers or controller"));
        return nullptr;
    }

    uintptr_t identity = SafeReadPtr((uintptr_t)controller + 0x10);
    if (!identity) {
        if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Step 0: Failed to read CEntityIdentity"));
        return nullptr;
    }

    uint32_t handle = *(uint32_t*)(identity + 0x10);
    if (g_Lua) {
        sprintf_s(buf, "[Cmd] Step 0 OK, Handle: %X", handle);
        g_Lua->GetLogClient().SendLog(std::string(buf));
    }

    int32_t tick = 0;
    
    if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Step 1: GetCUserCmdTick"));
    GetCUserCmdTick(pCCSGOInput, &handle, &tick);
    
    if (g_Lua) {
        sprintf_s(buf, "[Cmd] Step 1 OK, Tick: %d", tick);
        g_Lua->GetLogClient().SendLog(std::string(buf));
    }

    if (tick <= 0) return nullptr;

    if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Step 2: อ่าน first_array"));
    void** first_array = *(void***)pCCSGOInput;
    if (!first_array) {
        if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] first_array is NULL"));
        return nullptr;
    }

    if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Step 3: GetCUserCmdArray"));
    void* arr = GetCUserCmdArray(first_array, tick - 1);
    if (!arr) {
        if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] GetCUserCmdArray returned NULL"));
        return nullptr;
    }

    if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Step 4: Resolve Sequence"));
    uint32_t seq = *(uint32_t*)((uintptr_t)arr + 0x59A8); 

    if (g_Lua) g_Lua->GetLogClient().SendLog(std::string("[Cmd] Step 5: GetCUserCmdBySequenceNumber"));
    return GetCUserCmdBySequenceNumber(pCCSGOInput, &handle, seq);
}
