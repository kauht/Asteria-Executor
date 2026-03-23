#pragma once
#include <Windows.h>
#include <cstdint>

// Resolved once on DLL attach. All fields are safe to read from any thread
// after Init() returns.
class GameContext {
public:
    static GameContext& Get() {
        static GameContext s_instance;
        return s_instance;
    }

    bool Init();
    void* GetCurrentUserCmd(void* controller);

    // ── Module bases ────────────────────────────────────────────────────────
    uintptr_t client_base  = 0;
    uintptr_t engine_base  = 0;
    uintptr_t tier0_base   = 0;

    // ── Key pointers (absolute addresses) ───────────────────────────────────
    uintptr_t entity_list       = 0;  // dwEntityList
    uintptr_t local_controller  = 0;  // dwLocalPlayerController
    uintptr_t view_angles_ptr   = 0;  // dwViewAngles
    void*     create_move_addr  = nullptr; // CreateMove pattern scan target

    // ── CCSGOInput Helpers ───────────────────────────────────────────────────
    void* pCCSGOInput = nullptr; // Gets resolved to instance pointer
    
    using FnGetCUserCmdTick = void(__fastcall*)(void* thisptr, uint32_t* pHandle, int32_t* pTick);
    FnGetCUserCmdTick GetCUserCmdTick = nullptr;

    using FnGetCUserCmdArray = void*(__fastcall*)(void**, int);
    FnGetCUserCmdArray GetCUserCmdArray = nullptr;

    using FnGetCUserCmdBySequenceNumber = void*(__fastcall*)(void* thisptr, uint32_t* pHandle, uint32_t seq);
    FnGetCUserCmdBySequenceNumber GetCUserCmdBySequenceNumber = nullptr;

    // ── ConMsg ───────────────────────────────────────────────────────────────
    using ConMsgFn = void(__cdecl*)(const char*, ...);
    ConMsgFn ConMsg = nullptr;

    bool ready = false;

private:
    GameContext() = default;
};
