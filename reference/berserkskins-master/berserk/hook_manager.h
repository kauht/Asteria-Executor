#pragma once
#include <cstdint>
#include "kiero/minhook/include/MinHook.h"

// Base hook object template class for managing individual hooks
template <typename T>
class CBaseHookObject
{
public:
	CBaseHookObject() = default;
	~CBaseHookObject() = default;

	/// Setup hook and replace function
	/// @param pFunction - pointer to the function to hook
	/// @param pDetour - pointer to the detour function
	/// @returns: true if hook has been successfully created, false otherwise
	bool Create(void* pFunction, void* pDetour)
	{
		if (pFunction == nullptr || pDetour == nullptr)
			return false;

		pBaseFn = pFunction;
		pReplaceFn = pDetour;

		if (const MH_STATUS status = MH_CreateHook(pBaseFn, pReplaceFn, &pOriginalFn); status != MH_OK)
		{
			// Failed to create hook
			return false;
		}

		if (!Replace())
			return false;

		return true;
	}

	/// Patch memory to jump to our function instead of original
	/// @returns: true if hook has been successfully applied, false otherwise
	bool Replace()
	{
		// Check if hook has been created
		if (pBaseFn == nullptr)
			return false;

		// Check that function isn't already hooked
		if (bIsHooked)
			return false;

		if (const MH_STATUS status = MH_EnableHook(pBaseFn); status != MH_OK)
		{
			// Failed to enable hook
			return false;
		}

		// Switch hook state
		bIsHooked = true;
		return true;
	}

	/// Restore original function call and cleanup hook data
	/// @returns: true if hook has been successfully removed, false otherwise
	bool Remove()
	{
		// Restore it at first
		if (!Restore())
			return false;

		if (const MH_STATUS status = MH_RemoveHook(pBaseFn); status != MH_OK)
		{
			// Failed to remove hook
			return false;
		}

		return true;
	}

	/// Restore patched memory to original function call
	/// @returns: true if hook has been successfully restored, false otherwise
	bool Restore()
	{
		// Check that function is hooked
		if (!bIsHooked)
			return false;

		if (const MH_STATUS status = MH_DisableHook(pBaseFn); status != MH_OK)
		{
			// Failed to restore hook
			return false;
		}

		// Switch hook state
		bIsHooked = false;
		return true;
	}

	/// @returns: original, unwrapped function that would be called without the hook
	inline T GetOriginal()
	{
		return reinterpret_cast<T>(pOriginalFn);
	}

	/// @returns: true if hook is applied at the time, false otherwise
	inline bool IsHooked() const
	{
		return bIsHooked;
	}

private:
	// Current hook state
	bool bIsHooked = false;
	// Function base handle
	void* pBaseFn = nullptr;
	// Function that being replace the original call
	void* pReplaceFn = nullptr;
	// Original function
	void* pOriginalFn = nullptr;
};

// Hook Manager Namespace
namespace H
{
	/// Initialize MinHook and setup all hooks
	/// @returns: true if all hooks were successfully created, false otherwise
	bool Setup();

	/// Disable and remove all hooks, uninitialize MinHook
	void Destroy();

	// Example hook objects - add your hooks here
	// inline CBaseHookObject<decltype(&YourFunction)> hkYourFunction = {};
}



