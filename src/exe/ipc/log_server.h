#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

class LogServer {
public:
    LogServer();
    ~LogServer();

    bool Start();
    void Stop();

    std::vector<std::string> GetLogs();
    void ClearLogs();

private:
    void Run();

    HANDLE hPipe;
    std::thread workerThread;
    bool bRunning;
    std::vector<std::string> logs;
    std::mutex logsMutex;
};
