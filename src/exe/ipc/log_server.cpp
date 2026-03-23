#include "log_server.h"
#include "ipc_def.h"

LogServer::LogServer() : hPipe(INVALID_HANDLE_VALUE), bRunning(false) {}

LogServer::~LogServer() {
    Stop();
}

bool LogServer::Start() {
    if (bRunning) return true;

    hPipe = CreateNamedPipeA(
        LOG_PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        MAX_BUFFER_SIZE,
        MAX_BUFFER_SIZE,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        return false;
    }

    bRunning = true;
    workerThread = std::thread(&LogServer::Run, this);
    return true;
}

void LogServer::Stop() {
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

std::vector<std::string> LogServer::GetLogs() {
    std::lock_guard<std::mutex> lock(logsMutex);
    return logs;
}

void LogServer::ClearLogs() {
    std::lock_guard<std::mutex> lock(logsMutex);
    logs.clear();
}

void LogServer::Run() {
    char buffer[MAX_BUFFER_SIZE];
    DWORD bytesRead = 0;

    while (bRunning) {
        // Always ensure we have a valid named pipe handle before waiting for client
        if (hPipe == INVALID_HANDLE_VALUE) {
            hPipe = CreateNamedPipeA(
                LOG_PIPE_NAME,
                PIPE_ACCESS_INBOUND,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                1,
                MAX_BUFFER_SIZE,
                MAX_BUFFER_SIZE,
                0,
                NULL
            );
            if (hPipe == INVALID_HANDLE_VALUE) {
                Sleep(200);
                continue;
            }
        }

        BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            while (ReadFile(hPipe, buffer, MAX_BUFFER_SIZE - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0';
                std::lock_guard<std::mutex> lock(logsMutex);
                logs.push_back(std::string(buffer));
            }
        }

        // Disconnect and close this pipe instance — next iteration recreates it
        // so the DLL client can always reconnect fresh
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
        hPipe = INVALID_HANDLE_VALUE;

        if (!bRunning) break;
        Sleep(50); // brief gap before recreating pipe
    }
}

