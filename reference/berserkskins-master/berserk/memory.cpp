#include "memory.h"
#include <string>
#include <unordered_map>
#include "CRT.h"
#include "pe64.h"
#define MEM_STACKALLOC(SIZE) _malloca(SIZE)
#define MEM_STACKFREE(MEMORY) static_cast<void>(0)
std::unordered_map<std::string, std::uint8_t*> patternCache = {};

namespace M {

	void* GetModuleBaseHandle(const wchar_t* wszModuleName)
	{
		const _PEB* pPEB = reinterpret_cast<_PEB*>(__readgsqword(0x60));

		if (wszModuleName == nullptr)
			return pPEB->ImageBaseAddress;

		void* pModuleBase = nullptr;
		for (LIST_ENTRY* pListEntry = pPEB->Ldr->InMemoryOrderModuleList.Flink; pListEntry != &pPEB->Ldr->InMemoryOrderModuleList; pListEntry = pListEntry->Flink)
		{
			const _LDR_DATA_TABLE_ENTRY* pEntry = CONTAINING_RECORD(pListEntry, _LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

			if (pEntry->FullDllName.Buffer != nullptr && CRT::StringCompare(wszModuleName, pEntry->BaseDllName.Buffer) == 0)
			{
				pModuleBase = pEntry->DllBase;
				break;
			}
		}

		if (pModuleBase == nullptr)
			printf("dule not found: %ls\n", wszModuleName);

		return pModuleBase;
	}
	std::uint8_t* FindPattern(const wchar_t* wszModuleName, const char* szPattern)
	{
		std::string pattern = std::string(szPattern);
		if (patternCache.contains(pattern))
			return patternCache.at(pattern);
		// convert pattern string to byte array
		const std::size_t nApproximateBufferSize = (CRT::StringLength(szPattern) >> 1U) + 1U;
		std::uint8_t* arrByteBuffer = static_cast<std::uint8_t*>(MEM_STACKALLOC(nApproximateBufferSize));
		char* szMaskBuffer = static_cast<char*>(MEM_STACKALLOC(nApproximateBufferSize));
		PatternToBytes(szPattern, arrByteBuffer, szMaskBuffer);

		// @test: use search with straight in-place conversion? do not think it will be faster, cuz of bunch of new checks that gonna be performed for each iteration
		std::uint8_t* result = FindPattern(wszModuleName, reinterpret_cast<const char*>(arrByteBuffer), szMaskBuffer);
		if (result == nullptr)
		{
			std::string message = "Failed to find pattern: " + std::string(szPattern);
			printf("%s\n", message.c_str());
		}
		patternCache.insert({ pattern, result });

		return result;
	}

	std::uint8_t* FindPattern(const wchar_t* wszModuleName, const char* szBytePattern, const char* szByteMask)
	{
		const void* hModuleBase = GetModuleBaseHandle(wszModuleName);

		if (hModuleBase == nullptr)
		{
			printf("failed to get module base: %ls\n", wszModuleName);
			return nullptr;
		}

		const auto pBaseAddress = static_cast<const std::uint8_t*>(hModuleBase);

		const auto pIDH = static_cast<const IMAGE_DOS_HEADER*>(hModuleBase);
		if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
		{
			printf("failed to get module size, image is invalid\n");
			return nullptr;
		}

		const auto pINH = reinterpret_cast<const IMAGE_NT_HEADERS*>(pBaseAddress + pIDH->e_lfanew);
		if (pINH->Signature != IMAGE_NT_SIGNATURE)
		{
			printf("failed to get module size, image is invalid\n");
			return nullptr;
		}

		const std::uint8_t* arrByteBuffer = reinterpret_cast<const std::uint8_t*>(szBytePattern);
		const std::size_t nByteCount = CRT::StringLength(szByteMask);

		std::uint8_t* pFoundAddress = nullptr;

		// perform little overhead to keep all patterns unique
		pFoundAddress = FindPatternEx(pBaseAddress, pINH->OptionalHeader.SizeOfImage, arrByteBuffer, nByteCount, szByteMask);

		if (pFoundAddress == nullptr)
		{
			char* szPattern = static_cast<char*>(MEM_STACKALLOC((nByteCount << 1U) + nByteCount));
			[[maybe_unused]] const std::size_t nConvertedPatternLength = BytesToPattern(arrByteBuffer, nByteCount, szPattern);

			printf("pattern not found \n");

			MEM_STACKFREE(szPattern);
		}

		return pFoundAddress;
	}

	std::uint8_t* FindPatternEx(const std::uint8_t* pRegionStart, const std::size_t nRegionSize, const std::uint8_t* arrByteBuffer, const std::size_t nByteCount, const char* szByteMask)
	{
		std::uint8_t* pCurrentAddress = const_cast<std::uint8_t*>(pRegionStart);
		const std::uint8_t* pRegionEnd = pRegionStart + nRegionSize - nByteCount;
		const bool bIsMaskUsed = (szByteMask != nullptr);

		while (pCurrentAddress < pRegionEnd)
		{
			// check the first byte before entering the loop, otherwise if there two consecutive bytes of first byte in the buffer, we may skip both and fail the search
			if ((bIsMaskUsed && *szByteMask == '?') || *pCurrentAddress == *arrByteBuffer)
			{
				if (nByteCount == 1)
					return pCurrentAddress;

				// compare the least byte sequence and continue on wildcard or skip forward on first mismatched byte
				std::size_t nComparedBytes = 0U;
				while ((bIsMaskUsed && szByteMask[nComparedBytes + 1U] == '?') || pCurrentAddress[nComparedBytes + 1U] == arrByteBuffer[nComparedBytes + 1U])
				{
					// check does byte sequence match
					if (++nComparedBytes == nByteCount - 1U)
						return pCurrentAddress;
				}

				// skip non suitable bytes
				pCurrentAddress += nComparedBytes;
			}

			++pCurrentAddress;
		}

		return nullptr;
	}


	std::size_t BytesToPattern(const std::uint8_t* pByteBuffer, const std::size_t nByteCount, char* szOutBuffer)
	{
		char* szCurrentPattern = szOutBuffer;

		for (std::size_t i = 0U; i < nByteCount; i++)
		{
			// manually convert byte to chars
			const char* szHexByte = &CRT::_TWO_DIGITS_HEX_LUT[pByteBuffer[i] * 2U];
			*szCurrentPattern++ = szHexByte[0];
			*szCurrentPattern++ = szHexByte[1];
			*szCurrentPattern++ = ' ';
		}
		*--szCurrentPattern = '\0';

		return szCurrentPattern - szOutBuffer;
	}
	std::size_t PatternToBytes(const char* szPattern, std::uint8_t* pOutByteBuffer, char* szOutMaskBuffer)
	{
		std::uint8_t* pCurrentByte = pOutByteBuffer;

		while (*szPattern != '\0')
		{
			// check is a wildcard
			if (*szPattern == '?')
			{
				++szPattern;
				// ignore that
				* pCurrentByte++ = 0U;
				*szOutMaskBuffer++ = '?';
			}
			// check is not space
			else if (*szPattern != ' ')
			{
				// convert two consistent numbers in a row to byte value
				std::uint8_t uByte = static_cast<std::uint8_t>(CRT::CharToHexInt(*szPattern) << 4);

				++szPattern;


				uByte |= static_cast<std::uint8_t>(CRT::CharToHexInt(*szPattern));

				*pCurrentByte++ = uByte;
				*szOutMaskBuffer++ = 'x';
			}

			++szPattern;
		}

		// zero terminate both buffers
		*pCurrentByte = 0U;
		*szOutMaskBuffer = '\0';

		return pCurrentByte - pOutByteBuffer;
	}
	void* GetExportAddress(const void* hModuleBase, const char* szProcedureName)
	{
		const auto pBaseAddress = static_cast<const std::uint8_t*>(hModuleBase);

		const auto pIDH = static_cast<const IMAGE_DOS_HEADER*>(hModuleBase);
		if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
			return nullptr;

		const auto pINH = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBaseAddress + pIDH->e_lfanew);
		if (pINH->Signature != IMAGE_NT_SIGNATURE)
			return nullptr;

		const IMAGE_OPTIONAL_HEADER64* pIOH = &pINH->OptionalHeader;
		const std::uintptr_t nExportDirectorySize = pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		const std::uintptr_t uExportDirectoryAddress = pIOH->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

		if (nExportDirectorySize == 0U || uExportDirectoryAddress == 0U)
		{
			return nullptr;
		}

		const auto pIED = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(pBaseAddress + uExportDirectoryAddress);
		const auto pNamesRVA = reinterpret_cast<const std::uint32_t*>(pBaseAddress + pIED->AddressOfNames);
		const auto pNameOrdinalsRVA = reinterpret_cast<const std::uint16_t*>(pBaseAddress + pIED->AddressOfNameOrdinals);
		const auto pFunctionsRVA = reinterpret_cast<const std::uint32_t*>(pBaseAddress + pIED->AddressOfFunctions);

		// Perform binary search to find the export by name
		std::size_t nRight = pIED->NumberOfNames, nLeft = 0U;
		while (nRight != nLeft)
		{
			// Avoid INT_MAX/2 overflow
			const std::size_t uMiddle = nLeft + ((nRight - nLeft) >> 1U);
			const int iResult = CRT::StringCompare(szProcedureName, reinterpret_cast<const char*>(pBaseAddress + pNamesRVA[uMiddle]));

			if (iResult == 0)
			{
				const std::uint32_t uFunctionRVA = pFunctionsRVA[pNameOrdinalsRVA[uMiddle]];


				// Check if it's a forwarded export
				if (uFunctionRVA >= uExportDirectoryAddress && uFunctionRVA - uExportDirectoryAddress < nExportDirectorySize)
				{
					// Forwarded exports are not supported
					break;
				}

				return const_cast<std::uint8_t*>(pBaseAddress) + uFunctionRVA;
			}

			if (iResult > 0)
				nLeft = uMiddle + 1;
			else
				nRight = uMiddle;
		}

		// Export not found
		return nullptr;
	}
}
