#pragma once
#include <map>
#include <string>
ImU32 GetColorWithAlpha(ImU32 color);
ImU32 GetRarityColor(int rarity);
struct TabAnimation {
    float hover_alpha = 0.0f;
    float active_alpha = 0.0f;
    float press_scale = 1.0f;
};

namespace W{
	void DrawLogo();
	bool AnimatedTabButton(const char* label, int tab_index, bool is_active, ImVec2 size);
	
	bool AnimatedButton(const char* label, int button_index, ImVec2 size, bool is_tab_mode = false, bool* tab_state = nullptr);

	void RenderSkinManagerWindow();
	
	std::string GetLocalizedWeaponName(const std::string& weaponTag);
	std::string GetLocalizedSkinName(const std::string& skinTag);
	std::string GetSkinImagePath(const std::string& weaponTag, const std::string& skinToken);
	std::string GetRarityName(int rarity);
	
	bool DrawSkinTile(const char* text, const ImVec2& size, int rarity, bool isSelected = false);

}