#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
namespace M {
    [[nodiscard]] void* GetModuleBaseHandle(const wchar_t* wszModuleName);
    [[nodiscard]] std::uint8_t* FindPattern(const wchar_t* wszModuleName, const char* szPattern);
    /// naive style pattern byte comparison in a specific module
    /// 
    /// @param[in] wszModuleName module name where to search for pattern
    /// @param[in] szBytePattern naive style pattern, e.g. "\x55\x8B\x40\x00\x30", wildcard bytes value ignored
    /// @param[in] szByteMask wildcard mask for byte array, e.g. "xxx?x", should always correspond to bytes count
    /// @returns: pointer to address of the first found occurrence with equal byte sequence on success, null otherwise
    [[nodiscard]] std::uint8_t* FindPattern(const wchar_t* wszModuleName, const char* szBytePattern, const char* szByteMask);

    [[nodiscard]] std::uint8_t* FindPatternEx(const std::uint8_t* pRegionStart, const std::size_t nRegionSize, const std::uint8_t* arrByteBuffer, const std::size_t nByteCount, const char* szByteMask = nullptr);

    std::size_t PatternToBytes(const char* szPattern, std::uint8_t* pOutByteBuffer, char* szOutMaskBuffer);
    /// convert byte array to ida-style pattern
    /// @param[in] pByteBuffer buffer of bytes to convert
    /// @param[in] nByteCount count of bytes to convert
    /// @param[out] szOutBuffer output for converted pattern
    /// @returns: length of the converted ida-style pattern, not including the terminating null
    std::size_t BytesToPattern(const std::uint8_t* pByteBuffer, const std::size_t nByteCount, char* szOutBuffer);
    template <typename T = std::uint8_t>
    [[nodiscard]] T* GetAbsoluteAddress(T* pRelativeAddress, int nPreOffset = 0x0, int nPostOffset = 0x0)
    {
        pRelativeAddress += nPreOffset;
        pRelativeAddress += sizeof(std::int32_t) + *reinterpret_cast<std::int32_t*>(pRelativeAddress);
        pRelativeAddress += nPostOffset;
        return pRelativeAddress;
    }
    /// resolve rip relative address
    /// @param[in] nAddressBytes as byte for the address we want to resolve
    /// @param[in] nRVAOffset offset of the relative address
    /// @param[in] nRIPOffset offset of the instruction pointer
    /// @returns: pointer to resolved address
    [[nodiscard]] void* GetExportAddress(const void* hModuleBase, const char* szProcedureName);

    [[nodiscard]] __forceinline std::uint8_t* ResolveRelativeAddress(std::uint8_t* nAddressBytes, std::uint32_t nRVAOffset, std::uint32_t nRIPOffset)
    {
        std::uint32_t nRVA = *reinterpret_cast<std::uint32_t*>(nAddressBytes + nRVAOffset);
        std::uint64_t nRIP = reinterpret_cast<std::uint64_t>(nAddressBytes) + nRIPOffset;

        return reinterpret_cast<std::uint8_t*>(nRVA + nRIP);
    }

    template <typename T, std::size_t nIndex, class CBaseClass, typename... Args_t>
    inline T CallVFunc(CBaseClass* thisptr, Args_t&&... argList)
    {
        using VirtualFn_t = T(__thiscall*)(CBaseClass*, Args_t...);
        return (*reinterpret_cast<VirtualFn_t**>(thisptr))[nIndex](thisptr, std::forward<Args_t>(argList)...);
    }

    template <typename T = void*>
    inline T get_v_method(void* class_, unsigned int index) {
        if (!class_)
            return T{};

        void** table = *static_cast<void***>(class_);
        if (!table)
            return T{};

        return reinterpret_cast<T>(table[index]);
    }

    template <typename T, typename... Args>
    inline T call_virtual(void* class_, unsigned int index, Args... args) {
        auto func = get_v_method<T(__thiscall*)(void*, Args...)>(class_, index);

        return func(class_, args...);
    }

}