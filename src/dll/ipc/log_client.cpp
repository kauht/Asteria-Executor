#include "log_client.h"
#include "common.hpp"

LogClient::LogClient() : hPipe(INVALID_HANDLE_VALUE) {}

LogClient::~LogClient() {
  if (hPipe != INVALID_HANDLE_VALUE) {
    CloseHandle(hPipe);
  }
}

bool LogClient::SendLog(const std::string &log) {
  // If pipe is not open (or was closed after a failed write), try to reconnect
  if (hPipe == INVALID_HANDLE_VALUE) {
    hPipe = CreateFileA(LOG_PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0,
                        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
      return false; // UI not listening yet, drop silently
    }
  }

  DWORD bytesWritten;
  BOOL success =
      WriteFile(hPipe, log.c_str(), (DWORD)log.length(), &bytesWritten, NULL);

  if (!success) {
    // Pipe broke (UI restarted / reconnected). Close handle so next call
    // retries.
    CloseHandle(hPipe);
    hPipe = INVALID_HANDLE_VALUE;
    // Retry once immediately with a fresh connection
    hPipe = CreateFileA(LOG_PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0,
                        NULL);
    if (hPipe != INVALID_HANDLE_VALUE) {
      WriteFile(hPipe, log.c_str(), (DWORD)log.length(), &bytesWritten, NULL);
    }
  }

  return success;
}
