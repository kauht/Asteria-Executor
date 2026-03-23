#include "sig_scanner.h"
#include <sstream>
#include <string>
#include <vector>

struct PatternByte {
  bool wildcard;
  uint8_t value;
};

static std::vector<PatternByte> ParsePattern(const char *pattern) {
  std::vector<PatternByte> bytes;
  std::istringstream stream(pattern);
  std::string token;

  while (stream >> token) {
    if (token == "?" || token == "??") {
      bytes.push_back({true, 0});
    } else {
      bytes.push_back({false, (uint8_t)strtoul(token.c_str(), nullptr, 16)});
    }
  }
  return bytes;
}

static void *ScanLoop(uint8_t *scanStart, uint8_t *scanEnd,
                      const std::vector<PatternByte> &parsed) {
  size_t patLen = parsed.size();
  __try {
    for (uint8_t *curr = scanStart; curr <= scanEnd; curr++) {
      bool found = true;
      for (size_t j = 0; j < patLen; j++) {
        if (!parsed[j].wildcard && curr[j] != parsed[j].value) {
          found = false;
          break;
        }
      }
      if (found) {
        return reinterpret_cast<void *>(curr);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
  return nullptr;
}

extern "C" __declspec(dllexport) void *ScanPattern(const char *moduleName,
                                                   const char *pattern) {
  HMODULE hModule = GetModuleHandleA(moduleName);
  if (!hModule)
    return nullptr;

  auto *dosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(hModule);
  auto *ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS *>(
      reinterpret_cast<uint8_t *>(hModule) + dosHeader->e_lfanew);

  uintptr_t base = reinterpret_cast<uintptr_t>(hModule);
  size_t size = ntHeaders->OptionalHeader.SizeOfImage;
  uint8_t *scanStart = reinterpret_cast<uint8_t *>(base);

  // Speed up and secure walk bounds by filtering .text section
  IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(ntHeaders);
  for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++) {
    if (strcmp((const char *)section->Name, ".text") == 0) {
      scanStart = reinterpret_cast<uint8_t *>(base + section->VirtualAddress);
      size = section->Misc.VirtualSize;
      break;
    }
  }

  auto parsed = ParsePattern(pattern);
  if (parsed.empty())
    return nullptr;

  uint8_t *scanEnd = scanStart + size - parsed.size();

  return ScanLoop(scanStart, scanEnd, parsed);
}
