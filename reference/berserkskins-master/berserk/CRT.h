#pragma once
#include <cstddef>
namespace CRT {
	constexpr char _TWO_DIGITS_HEX_LUT[] =
		"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"
		"202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F"
		"404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F"
		"606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F"
		"808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9F"
		"A0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
		"C0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
		"E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

	template <typename C> requires (std::is_same_v<C, char> || std::is_same_v<C, wchar_t>)
		constexpr std::size_t StringLength(const C* tszSource)
	{
		const C* tszSourceEnd = tszSource;

		while (*tszSourceEnd != C('\0'))
			++tszSourceEnd;

		return tszSourceEnd - tszSource;
	}
	[[nodiscard]] constexpr bool IsDigit(const std::uint8_t uChar)
	{
		return (uChar >= '0' && uChar <= '9');
	}
	[[nodiscard]] constexpr std::uint32_t CharToHexInt(const std::uint8_t uChar)
	{
		const std::uint8_t uCharLower = (uChar | ('a' ^ 'A'));
		return ((uCharLower >= 'a' && uCharLower <= 'f') ? (uCharLower - 'a' + 0xA) : (IsDigit(uChar) ? (uChar - '0') : 0x0));
	}
	template <typename C> requires (std::is_same_v<C, char> || std::is_same_v<C, wchar_t>)
		constexpr int StringCompare(const C* tszLeft, const C* tszRight)
	{
		if (tszLeft == nullptr)
			return -1;

		if (tszRight == nullptr)
			return 1;

		using ComparisonType_t = std::conditional_t<std::is_same_v<C, char>, std::uint8_t, std::conditional_t<sizeof(wchar_t) == 2U, std::int16_t, std::int32_t>>;

		ComparisonType_t nLeft, nRight;
		do
		{
			nLeft = static_cast<ComparisonType_t>(*tszLeft++);
			nRight = static_cast<ComparisonType_t>(*tszRight++);

			if (nLeft == C('\0'))
				break;
		} while (nLeft == nRight);

		return nLeft - nRight;
	}
}
