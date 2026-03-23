#pragma once
#include <windows.h>
#include <cstdint>

// Scans a loaded module for an IDA-style byte pattern (e.g. "48 89 5C 24 ?? 55 57")
// Returns the address of the first match, or nullptr if not found.
extern "C" __declspec(dllexport) void* ScanPattern(const char* moduleName, const char* pattern);
