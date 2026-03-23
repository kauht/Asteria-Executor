#pragma once
#include <windows.h>
#include <string>
#include <thread>
#include "lua_manager.h"

class IpcServer {
public:
    IpcServer(LuaManager& lua);
    ~IpcServer();

    bool Start();
    void Stop();

private:
    void Run();

    LuaManager& luaManager;
    HANDLE hPipe;
    std::thread workerThread;
    bool bRunning;
};
