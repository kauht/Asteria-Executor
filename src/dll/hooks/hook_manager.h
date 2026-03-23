#pragma once
#include <windows.h>
#include <functional>
#include <mutex>

// Forward declaration
struct lua_State;

class HookManager {
public:
    static HookManager& Get() {
        static HookManager instance;
        return instance;
    }

    bool Init();
    void Shutdown();

    // CreateMove Hook
    void SetCreateMoveCallback(int ref);
    bool HookCreateMove(void* pTarget);
    void UnhookCreateMove();

    // Getters for detour
    int GetCreateMoveCallback() const { return m_CreateMoveCallbackRef; }
    void* GetOriginalCreateMove() const { return m_OriginalCreateMove; }

private:
    HookManager() = default;
    ~HookManager() = default;

    int m_CreateMoveCallbackRef = -1; // LUA_NOREF
    void* m_OriginalCreateMove = nullptr;
    void* m_CreateMoveTarget = nullptr;

    std::mutex m_Mutex;
};
