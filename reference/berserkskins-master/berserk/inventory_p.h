#pragma once
#include <string>
#include <vector>
// Lightweight persistence for added CEconItems without external JSON libs.
// Stores a simple JSON array in a file and recreates items on next run.

struct PersistedEconItem
{
	uint64_t itemID = 0; // last assigned runtime id (for sync)
	unsigned int defIndex = 0;
	int quality = 0;
	int rarity = 0;
	float paintKit = 0.0f;
	float paintSeed = 0.0f;
	float paintWear = 0.0f;
	bool legacy = false;
	bool unusual = false;
	int statTrak = -1; // StatTrak count (-1 means not StatTrak)
	std::string weaponTag = ""; // Weapon localization tag (e.g., "weapon_ak47")
	std::string skinTag = ""; // Skin localization tag (e.g., "paintkit_ak47_asimov")
	std::string skinToken = ""; // Skin token name (e.g., "cu_ak47_asimov")
	std::string imagePath = ""; // Full path to skin image (e.g., "s2r://panorama/images/econ/default_generated/ak47_cu_ak47_asimov_light_png.vtex")
};

struct EquippedItemInfo
{
	uint64_t itemID = 0;
	int team = 0;
	int slot = 0;
	time_t timestamp = 0;
};

namespace InventoryPersistence
{
	// Append a record and rewrite the JSON file (simple, robust)
	bool Append(const PersistedEconItem& item, const std::string& filename = "added_items.json");
	// Same as Append but stores a known assigned itemID immediately
	bool AppendWithId(const PersistedEconItem& item, uint64_t itemID, const std::string& filename = "added_items.json");

	// Load file and recreate items via CEconItem + CCSPlayerInventory
	// Returns number of successfully created items
	int LoadAndRecreate(const std::string& filename = "added_items.json");

	// Append equipped item info to JSON file (removes old entries with same slot)
	bool AppendEquippedItem(uint64_t itemID, int team, int slot, const std::string& filename = "equipped_items.json");

	// Helper functions for equipped items
	std::vector<EquippedItemInfo> ReadEquippedItems(const std::string& filename = "equipped_items.json");
	bool WriteEquippedItems(const std::vector<EquippedItemInfo>& items, const std::string& filename = "equipped_items.json");

	// Find equipped item by team and slot
	EquippedItemInfo FindEquippedItem(int team, int slot, const std::string& filename = "equipped_items.json");

	// Equip all items from JSON file
	bool Equip();

	// Clear all JSON skin files
	bool ClearAllSkins();
	bool ClearAddedItems(const std::string& filename = "added_items.json");
	bool ClearEquippedItems(const std::string& filename = "equipped_items.json");

	// New JSON-based functions using nlohmann::json
	bool AppendJson(const PersistedEconItem& item, const std::string& filename = "added_items.json");
	std::vector<PersistedEconItem> LoadJsonItems(const std::string& filename = "added_items.json");
	bool SaveJsonItems(const std::vector<PersistedEconItem>& items, const std::string& filename = "added_items.json");

	bool init();
}




