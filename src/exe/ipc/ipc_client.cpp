#include "ipc_client.h"
#include "common.hpp"
#include <iostream>
#include <windows.h>

IpcClient::IpcClient() {}

IpcClient::~IpcClient() {}

bool IpcClient::SendCode(const std::string &code) {
  HANDLE hPipe = CreateFileA(PIPE_NAME, GENERIC_WRITE,
                             0, // No sharing
                             NULL, OPEN_EXISTING, 0, NULL);

  if (hPipe == INVALID_HANDLE_VALUE) {
    if (GetLastError() != ERROR_PIPE_BUSY) {
      // Pipe not found or something
      return false;
    }
    // Wait for pipe
    if (!WaitNamedPipeA(PIPE_NAME, 2000)) {
      return false;
    }
  }

  DWORD bytesWritten;
  BOOL success =
      WriteFile(hPipe, code.c_str(), (DWORD)code.length(), &bytesWritten, NULL);

  CloseHandle(hPipe);
  return success;
}
