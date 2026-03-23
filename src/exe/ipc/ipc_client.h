#pragma once
#include <string>

class IpcClient {
public:
    IpcClient();
    ~IpcClient();

    bool SendCode(const std::string& code);
};
