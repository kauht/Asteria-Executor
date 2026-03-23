#include "ipc_server.h"
#include "ipc_def.h"
#include <iostream>

IpcServer::IpcServer(LuaManager& lua) : luaManager(lua), hPipe(INVALID_HANDLE_VALUE), bRunning(false) {}

IpcServer::~IpcServer() {
    Stop();
}

bool IpcServer::Start() {
    if (bRunning) return true;

    hPipe = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND, // Only read from pipe
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, // Max instances
        MAX_BUFFER_SIZE,
        MAX_BUFFER_SIZE,
        0, // Default timeout
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        //std::cerr << "Failed to create named pipe. Error: " << GetLastError() << std::endl;
        return false;
    }

    bRunning = true;
    workerThread = std::thread(&IpcServer::Run, this);
    return true;
}

void IpcServer::Stop() {
    if (!bRunning) return;

    bRunning = false;
    if (hPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void IpcServer::Run() {
    char buffer[MAX_BUFFER_SIZE];
    DWORD bytesRead = 0;

    while (bRunning) {
        // Wait for client to connect
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            // Read from pipe
            while (ReadFile(hPipe, buffer, MAX_BUFFER_SIZE - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0'; // Null terminate
                std::string code(buffer);
                
                // Execute Lua code
                luaManager.ExecuteString(code);
            }
        }

        // Disconnect and wait for next client
        DisconnectNamedPipe(hPipe);
        if (hPipe == INVALID_HANDLE_VALUE) break; // Pipe closed
        Sleep(100); // Small sleep to avoid infinite tight loop if error
    }
}
