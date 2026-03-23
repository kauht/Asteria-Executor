#pragma once
#include <lua.hpp>
#include <string>
#include "ipc/log_client.h"

class LuaManager {
public:
    LuaManager();
    ~LuaManager();

    bool Init();
    void Shutdown();
    bool ExecuteString(const std::string& code);
    
    LogClient& GetLogClient() { return logClient; }
    lua_State* GetState() const { return L; }

private:
    lua_State* L;
    LogClient logClient;
};
