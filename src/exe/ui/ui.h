#pragma once
#include <string>

class IpcClient;
class LogServer;

void RenderUI(IpcClient& ipc, LogServer& logServer, std::string& statusText, char* codeBuffer, size_t codeBufferSize, const std::string& dllPath);
