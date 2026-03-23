#pragma once
#include <windows.h>
#include <string>

class LogClient {
public:
    LogClient();
    ~LogClient();

    bool SendLog(const std::string& log);

private:
    HANDLE hPipe;
};
