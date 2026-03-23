#include "inventory_p.h"

#include <vector>
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <Windows.h>
#include <ShlObj.h> 
#include "CCPlayerInventory.h"
#include "CEconItem.h"
#include "i_client.h"
#include <filesystem>
#include "json.hpp"
#include "skins.h"
using json = nlohmann::json;
// Forward declare to avoid heavy includes and redefinitions
class CEconItem;
namespace skins { void AddEconItemToList(CEconItem* pItem, float paintKit, float paintSeed, float paintWear, bool legacy); }
enum EEconItemQuality {
	IQ_UNDEFINED = -1,
	IQ_NORMAL,
	IQ_GENUINE,
	IQ_VINTAGE,
	IQ_UNUSUAL,
	IQ_UNIQUE,
	IQ_COMMUNITY,
	IQ_DEVELOPER,
	IQ_SELFMADE,
	IQ_CUSTOMIZED,
	IQ_STRANGE,
	IQ_COMPLETED,
	IQ_HAUNTED,
	IQ_TOURNAMENT,
	IQ_FAVORED
};
static std::string Trim(const std::string& s)
{
	std::string r = s;
	r.erase(r.begin(), std::find_if(r.begin(), r.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	r.erase(std::find_if(r.rbegin(), r.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), r.end());
	return r;
}

static std::string GetBaseConfigDir()
{
	char documentsPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath))) {
		std::string path = std::string(documentsPath) + "\\Berserk\\Skins\\";
		std::filesystem::create_directories(path);
		return path;
	}
	return "Configs\\";
}

static void EnsureDirectoryExists(const std::string& path)
{
	// Create nested directories (best-effort)
	std::string partial;
	partial.reserve(path.size());
	for (size_t i = 0; i < path.size(); ++i) {
		char c = path[i];
		partial.push_back(c);
		if (c == '\\' || i + 1 == path.size()) {
			// Skip drive like "C:\\"
			if (partial.size() <= 3) continue;
			CreateDirectoryA(partial.c_str(), nullptr);
		}
	}
}

static std::string ResolvePath(const std::string& filename)
{
	// If absolute (has drive letter or starts with \\) return as is
	if (filename.size() >= 2 && filename[1] == ':') return filename;
	if (!filename.empty() && (filename[0] == '\\' || filename[0] == '/')) return filename;
	const std::string base = GetBaseConfigDir();
	EnsureDirectoryExists(base);
	std::string full = base;
	if (!full.empty() && full.back() != '\\') full.push_back('\\');
	full += filename;
	return full;
}

static std::string Escape(const std::string& s)
{
	std::string out;
	out.reserve(s.size());
	for (char c : s) {
		if (c == '"' || c == '\\') out.push_back('\\');
		out.push_back(c);
	}
	return out;
}

static std::string ItemToJson(const PersistedEconItem& it)
{
	// Используем nlohmann::json для более надежной сериализации
	json j;
	j["itemID"] = it.itemID;
	j["defIndex"] = it.defIndex;
	j["quality"] = it.quality;
	j["rarity"] = it.rarity;
	j["paintKit"] = it.paintKit;
	j["paintSeed"] = it.paintSeed;
	j["paintWear"] = it.paintWear;
	j["legacy"] = it.legacy;
	j["unusual"] = it.unusual;
	j["statTrak"] = it.statTrak;
	j["weaponTag"] = it.weaponTag;
	j["skinTag"] = it.skinTag;
	j["skinToken"] = it.skinToken;
	j["imagePath"] = it.imagePath;
	return j.dump();
}

static bool ParseBool(const std::string& v, bool def)
{
	if (v == "true") return true;
	if (v == "false") return false;
	return def;
}

static std::string EquippedItemToJson(const EquippedItemInfo& item)
{
	char buf[256];
	snprintf(buf, sizeof(buf),
		"{\"itemID\":%llu,\"team\":%d,\"slot\":%d,\"timestamp\":%lld}",
		item.itemID, item.team, item.slot, (long long)item.timestamp);
	return std::string(buf);
}

static bool ParseEquippedBool(const std::string& v, bool def)
{
	if (v == "true") return true;
	if (v == "false") return false;
	return def;
}

static std::vector<EquippedItemInfo> ParseEquippedJsonArray(const std::string& text)
{
	std::vector<EquippedItemInfo> out;
	std::string s = Trim(text);
	if (s.empty() || s.front() != '[' || s.back() != ']') return out;
	s = s.substr(1, s.size() - 2);

	size_t pos = 0;
	while (pos < s.size()) {
		size_t start = s.find('{', pos);
		if (start == std::string::npos) break;
		size_t depth = 0;
		size_t end = start;
		for (; end < s.size(); ++end) {
			if (s[end] == '{') depth++;
			else if (s[end] == '}') { depth--; if (depth == 0) { end++; break; } }
		}
		if (end <= start) break;
		std::string obj = s.substr(start, end - start);

		EquippedItemInfo item{};
		size_t o = 1;
		while (o < obj.size() && obj[o] != '}') {
			size_t kq1 = obj.find('"', o);
			if (kq1 == std::string::npos) break;
			size_t kq2 = obj.find('"', kq1 + 1);
			if (kq2 == std::string::npos) break;
			std::string key = obj.substr(kq1 + 1, kq2 - kq1 - 1);
			size_t colon = obj.find(':', kq2 + 1);
			if (colon == std::string::npos) break;
			size_t val_end = colon + 1;
			while (val_end < obj.size() && std::isspace((unsigned char)obj[val_end])) val_end++;
			size_t vstart = val_end;
			while (val_end < obj.size() && obj[val_end] != ',' && obj[val_end] != '}') val_end++;
			std::string raw = Trim(obj.substr(vstart, val_end - vstart));

			if (key == "itemID") item.itemID = std::stoull(raw);
			else if (key == "team") item.team = std::stoi(raw);
			else if (key == "slot") item.slot = std::stoi(raw);
			else if (key == "timestamp") item.timestamp = (time_t)std::stoll(raw);

			o = val_end;
			if (o < obj.size() && obj[o] == ',') o++;
		}
		out.push_back(item);
		pos = end;
	}
	return out;
}

static std::vector<PersistedEconItem> ParseJsonArray(const std::string& text)
{
	// Very small permissive parser for array of flat objects
	std::vector<PersistedEconItem> out;
	std::string s = Trim(text);
	if (s.empty() || s.front() != '[' || s.back() != ']') return out;
	s = s.substr(1, s.size() - 2);

	size_t pos = 0;
	while (pos < s.size()) {
		// find next object
		size_t start = s.find('{', pos);
		if (start == std::string::npos) break;
		size_t depth = 0;
		size_t end = start;
		for (; end < s.size(); ++end) {
			if (s[end] == '{') depth++;
			else if (s[end] == '}') { depth--; if (depth == 0) { end++; break; } }
		}
		if (end <= start) break;
		std::string obj = s.substr(start, end - start);

		PersistedEconItem item{};
		// split by commas at depth 1
		size_t o = 1; // skip leading '{'
		while (o < obj.size() && obj[o] != '}') {
			// key
			size_t kq1 = obj.find('"', o);
			if (kq1 == std::string::npos) break;
			size_t kq2 = obj.find('"', kq1 + 1);
			if (kq2 == std::string::npos) break;
			std::string key = obj.substr(kq1 + 1, kq2 - kq1 - 1);
			size_t colon = obj.find(':', kq2 + 1);
			if (colon == std::string::npos) break;
			size_t val_end = colon + 1;
			// value can be number/bool/string (we only use numbers/bools)
			while (val_end < obj.size() && std::isspace((unsigned char)obj[val_end])) val_end++;
			size_t vstart = val_end;
			// read until ',' or '}' at depth 1
			while (val_end < obj.size() && obj[val_end] != ',' && obj[val_end] != '}') val_end++;
			std::string raw = Trim(obj.substr(vstart, val_end - vstart));
			// assign
			if (key == "itemID") item.itemID = std::stoull(raw);
			else if (key == "defIndex") item.defIndex = (unsigned int)std::stoul(raw);
			else if (key == "quality") item.quality = std::stoi(raw);
			else if (key == "rarity") item.rarity = std::stoi(raw);
			else if (key == "paintKit") item.paintKit = (float)std::stof(raw);
			else if (key == "paintSeed") item.paintSeed = (float)std::stof(raw);
			else if (key == "paintWear") item.paintWear = (float)std::stof(raw);
			else if (key == "legacy") item.legacy = ParseBool(raw, false);
			else if (key == "unusual") item.unusual = ParseBool(raw, false);
			else if (key == "statTrak") item.statTrak = std::stoi(raw);
			else if (key == "weaponTag") {
				// Убираем кавычки для строковых значений
				if (raw.length() >= 2 && raw[0] == '"' && raw[raw.length()-1] == '"') {
					item.weaponTag = raw.substr(1, raw.length() - 2);
				} else {
					item.weaponTag = raw;
				}
			}
			else if (key == "skinTag") {
				if (raw.length() >= 2 && raw[0] == '"' && raw[raw.length()-1] == '"') {
					item.skinTag = raw.substr(1, raw.length() - 2);
				} else {
					item.skinTag = raw;
				}
			}
			else if (key == "skinToken") {
				if (raw.length() >= 2 && raw[0] == '"' && raw[raw.length()-1] == '"') {
					item.skinToken = raw.substr(1, raw.length() - 2);
				} else {
					item.skinToken = raw;
				}
			}
			else if (key == "imagePath") {
				if (raw.length() >= 2 && raw[0] == '"' && raw[raw.length()-1] == '"') {
					item.imagePath = raw.substr(1, raw.length() - 2);
				} else {
					item.imagePath = raw;
				}
			}
			// move
			o = val_end;
			if (o < obj.size() && obj[o] == ',') o++;
		}
		out.push_back(item);
		pos = end;
	}
	return out;
}




static std::vector<PersistedEconItem> ReadAll(const std::string& filename)
{
	std::ifstream in(ResolvePath(filename));
	if (!in.is_open()) return {};
	std::stringstream ss; ss << in.rdbuf();
	return ParseJsonArray(ss.str());
}

static bool WriteAll(const std::vector<PersistedEconItem>& items, const std::string& filename)
{
	std::ofstream out(ResolvePath(filename), std::ios::trunc);
	if (!out.is_open()) return false;
	out << "[\n";
	for (size_t i = 0; i < items.size(); ++i) {
		out << "  " << ItemToJson(items[i]);
		if (i + 1 < items.size()) out << ",";
		out << "\n";
	}
	out << "]\n";
	return true;
}
class CCSPlayerInventory;
namespace InventoryPersistence
{
	bool AppendWithId(const PersistedEconItem& item, uint64_t itemID, const std::string& filename)
	{
		PersistedEconItem copy = item;
		copy.itemID = itemID;
		return Append(copy, filename);
	}
	bool Equip()
	{
		CCSInventoryManager* pInventory = CCSInventoryManager::GetInstance();
		if (!pInventory) {
			std::cout << "[ERROR] Failed to get inventory instance" << std::endl;
			return false;
		}

		// Read all equipped items from JSON
		auto equippedItems = ReadEquippedItems();
		if (equippedItems.empty()) {
			std::cout << "[INFO] No equipped items found in JSON file" << std::endl;
			return true;
		}

		std::cout << "[INFO] Equipping " << equippedItems.size() << " items from JSON file..." << std::endl;

		int successCount = 0;
		for (const auto& item : equippedItems) {
			if (item.itemID == 0) continue; // Skip empty items

		/*	std::cout << "[INFO] Equipping item ID: " << item.itemID
				<< " to team: " << item.team
				<< " slot: " << item.slot << std::endl;*/

			// Call the game's equip function

			bool sucsses = pInventory->EquipItemInLoadout(item.team, item.slot, item.itemID);
			if (!sucsses) {
				std::cout << "[ERROR] Failed to equip item ID: " << item.itemID << std::endl;
				continue;
			}
			successCount++;
		}

		//std::cout << "[SUCCESS] Successfully equipped " << successCount << " items" << std::endl;
		return successCount > 0;
	}

	bool Append(const PersistedEconItem& item, const std::string& filename)
	{
		auto items = ReadAll(filename);
		items.push_back(item);
		return WriteAll(items, filename);
	}

	std::vector<EquippedItemInfo> ReadEquippedItems(const std::string& filename)
	{
		std::ifstream in(ResolvePath(filename));
		if (!in.is_open()) return {};
		std::stringstream ss; ss << in.rdbuf();
		return ParseEquippedJsonArray(ss.str());
	}

	bool WriteEquippedItems(const std::vector<EquippedItemInfo>& items, const std::string& filename)
	{
		std::ofstream out(ResolvePath(filename), std::ios::trunc);
		if (!out.is_open()) return false;
		out << "[\n";
		for (size_t i = 0; i < items.size(); ++i) {
			out << "  " << EquippedItemToJson(items[i]);
			if (i + 1 < items.size()) out << ",";
			out << "\n";
		}
		out << "]\n";
		return true;
	}

	EquippedItemInfo FindEquippedItem(int team, int slot, const std::string& filename)
	{
		auto items = ReadEquippedItems(filename);
		for (const auto& item : items) {
			if (item.team == team && item.slot == slot) {
				return item;
			}
		}
		return EquippedItemInfo{}; // Return empty item if not found
	}

	bool AppendEquippedItem(uint64_t itemID, int team, int slot, const std::string& filename)
	{
		// Read existing equipped items
		auto items = ReadEquippedItems(filename);

		// Remove any existing items with the same team AND slot
		items.erase(std::remove_if(items.begin(), items.end(),
			[team, slot](const EquippedItemInfo& item) {
				return item.team == team && item.slot == slot;
			}), items.end());

		// Add new item
		EquippedItemInfo newItem;
		newItem.itemID = itemID;
		newItem.team = team;
		newItem.slot = slot;
		newItem.timestamp = time(nullptr);
		items.push_back(newItem);

		// Write back to file
		return WriteEquippedItems(items, filename);
	}
	bool init() {
		CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
		if (!pInventory) return false;
		CEconItem* pItem = CEconItem::CreateInstance();
		if (!pItem) return false;
		return true;
	}
	int LoadAndRecreate(const std::string& filename)
	{
		auto list = ReadAll(filename);
		if (list.empty()) return 0;

		CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
		if (!pInventory) return 0;

		int created = 0;
		// Take one snapshot to maintain append order
		auto ids = pInventory->GetHighestIDs();
		uint64_t nextItemId = ids.first;
		uint32_t nextInvId = ids.second;
		for (auto e : list) {
			CEconItem* pItem = CEconItem::CreateInstance();
			if (!pItem) continue;
			// Assign monotonically increasing ids to place items at the end
			pItem->m_ulID = (e.itemID != 0 ? e.itemID : ++nextItemId);
			pItem->m_unInventory = ++nextInvId;
			pItem->m_unAccountID = (uint32_t)pInventory->GetOwner().m_id;
			pItem->m_unDefIndex = (uint16_t)e.defIndex;
			if (e.unusual) pItem->m_nQuality = IQ_UNUSUAL;
			pItem->m_nRarity = e.rarity;
			if (e.paintKit > 0.0f) {
				pItem->SetPaintKit(e.paintKit);
				pItem->SetPaintSeed(e.paintSeed);
				pItem->SetPaintWear(e.paintWear);
			}
			// Применяем StatTrak если он был сохранен
			if (e.statTrak >= 0) {
				pItem->SetStatTrak(e.statTrak);
				pItem->SetStatTrakType(0);
				if (pItem->m_nQuality != IQ_UNUSUAL) {
					pItem->m_nQuality = IQ_STRANGE;
				}
			}
			if (pInventory->AddEconItem(pItem)) {
				// also reflect in runtime vector used by skins module with legacy from JSON
				S::AddEconItemToList(pItem, e.paintKit, e.paintSeed, e.paintWear, e.legacy);
				// Update stored itemID if changed
				if (e.itemID != pItem->m_ulID) {
					e.itemID = pItem->m_ulID;
				}
				created++;
			}
		}
		// Write back possibly updated IDs
		WriteAll(list, filename);
		return created;
	}

	bool ClearAllSkins()
	{
		std::string baseDir = GetBaseConfigDir();
		int deletedCount = 0;

		try {
			// Clear added_items.json
			if (ClearAddedItems()) {
				deletedCount++;
				std::cout << "[SUCCESS] Cleared added_items.json" << std::endl;
			}

			// Clear equipped_items.json
			if (ClearEquippedItems()) {
				deletedCount++;
				std::cout << "[SUCCESS] Cleared equipped_items.json" << std::endl;
			}

			// Clear any other JSON files in the skins directory
			for (const auto& entry : std::filesystem::directory_iterator(baseDir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					std::filesystem::remove(entry.path());
					deletedCount++;
					std::cout << "[SUCCESS] Cleared " << entry.path().filename().string() << std::endl;
				}
			}

			std::cout << "[SUCCESS] Cleared " << deletedCount << " JSON skin files" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cout << "[ERROR] Failed to clear skin files: " << e.what() << std::endl;
			return false;
		}
	}

	bool ClearAddedItems(const std::string& filename)
	{
		try {
			std::string filePath = ResolvePath(filename);
			if (std::filesystem::exists(filePath)) {
				std::filesystem::remove(filePath);
				return true;
			}
			return true; // File doesn't exist, consider it cleared
		}
		catch (const std::exception& e) {
			std::cout << "[ERROR] Failed to clear added items: " << e.what() << std::endl;
			return false;
		}
	}

	bool ClearEquippedItems(const std::string& filename)
	{
		try {
			std::string filePath = ResolvePath(filename);
			if (std::filesystem::exists(filePath)) {
				std::filesystem::remove(filePath);
				return true;
			}
			return true; // File doesn't exist, consider it cleared
		}
		catch (const std::exception& e) {
			std::cout << "[ERROR] Failed to clear equipped items: " << e.what() << std::endl;
			return false;
		}
	}

	// JSON-based functions using nlohmann::json
	static json PersistedEconItemToJson(const PersistedEconItem& item)
	{
		json j;
		j["itemID"] = item.itemID;
		j["defIndex"] = item.defIndex;
		j["quality"] = item.quality;
		j["rarity"] = item.rarity;
		j["paintKit"] = item.paintKit;
		j["paintSeed"] = item.paintSeed;
		j["paintWear"] = item.paintWear;
		j["legacy"] = item.legacy;
		j["unusual"] = item.unusual;
		j["statTrak"] = item.statTrak;
		j["weaponTag"] = item.weaponTag;
		j["skinTag"] = item.skinTag;
		j["skinToken"] = item.skinToken;
		j["imagePath"] = item.imagePath;
		return j;
	}

	static PersistedEconItem JsonToPersistedEconItem(const json& j)
	{
		PersistedEconItem item;
		if (j.contains("itemID")) item.itemID = j["itemID"];
		if (j.contains("defIndex")) item.defIndex = j["defIndex"];
		if (j.contains("quality")) item.quality = j["quality"];
		if (j.contains("rarity")) item.rarity = j["rarity"];
		if (j.contains("paintKit")) item.paintKit = j["paintKit"];
		if (j.contains("paintSeed")) item.paintSeed = j["paintSeed"];
		if (j.contains("paintWear")) item.paintWear = j["paintWear"];
		if (j.contains("legacy")) item.legacy = j["legacy"];
		if (j.contains("unusual")) item.unusual = j["unusual"];
		if (j.contains("statTrak")) item.statTrak = j["statTrak"];
		if (j.contains("weaponTag")) item.weaponTag = j["weaponTag"];
		if (j.contains("skinTag")) item.skinTag = j["skinTag"];
		if (j.contains("skinToken")) item.skinToken = j["skinToken"];
		if (j.contains("imagePath")) item.imagePath = j["imagePath"];
		return item;
	}

	static json EquippedItemInfoToJson(const EquippedItemInfo& item)
	{
		json j;
		j["itemID"] = item.itemID;
		j["team"] = item.team;
		j["slot"] = item.slot;
		j["timestamp"] = item.timestamp;
		return j;
	}

	static EquippedItemInfo JsonToEquippedItemInfo(const json& j)
	{
		EquippedItemInfo item;
		if (j.contains("itemID")) item.itemID = j["itemID"];
		if (j.contains("team")) item.team = j["team"];
		if (j.contains("slot")) item.slot = j["slot"];
		if (j.contains("timestamp")) item.timestamp = j["timestamp"];
		return item;
	}

	// New JSON-based functions
	bool AppendJson(const PersistedEconItem& item, const std::string& filename)
	{
		try {
			json itemsArray;
			std::string filePath = ResolvePath(filename);

			// Read existing items
			if (std::filesystem::exists(filePath)) {
				std::ifstream file(filePath);
				if (file.is_open()) {
					file >> itemsArray;
					file.close();
				}
			}

			// Ensure it's an array
			if (!itemsArray.is_array()) {
				itemsArray = json::array();
			}

			// Add new item
			itemsArray.push_back(PersistedEconItemToJson(item));

			// Write back
			std::ofstream file(filePath);
			if (file.is_open()) {
				file << itemsArray.dump(4);
				file.close();
				return true;
			}
			return false;
		}
		catch (const std::exception& e) {
			std::cout << "[ERROR] Failed to append item: " << e.what() << std::endl;
			return false;
		}
	}

	std::vector<PersistedEconItem> LoadJsonItems(const std::string& filename)
	{
		std::vector<PersistedEconItem> items;
		try {
			std::string filePath = ResolvePath(filename);
			if (!std::filesystem::exists(filePath)) {
				return items;
			}

			std::ifstream file(filePath);
			if (!file.is_open()) {
				return items;
			}

			json itemsArray;
			file >> itemsArray;
			file.close();

			if (itemsArray.is_array()) {
				for (const auto& item : itemsArray) {
					items.push_back(JsonToPersistedEconItem(item));
				}
			}
		}
		catch (const std::exception& e) {
			std::cout << "[ERROR] Failed to load items: " << e.what() << std::endl;
		}
		return items;
	}

	bool SaveJsonItems(const std::vector<PersistedEconItem>& items, const std::string& filename)
	{
		try {
			json itemsArray = json::array();
			for (const auto& item : items) {
				itemsArray.push_back(PersistedEconItemToJson(item));
			}

			std::string filePath = ResolvePath(filename);
			std::ofstream file(filePath);
			if (file.is_open()) {
				file << itemsArray.dump(4);
				file.close();
				return true;
			}
			return false;
		}
		catch (const std::exception& e) {
			std::cout << "[ERROR] Failed to save items: " << e.what() << std::endl;
			return false;
		}
	}
}


