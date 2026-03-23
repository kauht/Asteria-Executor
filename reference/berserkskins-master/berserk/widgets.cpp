#define NOMINMAX
#include "imgui/imgui.h"
#include "sdk.h"
#include "gui.h"
#include <map>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include "imgui/imgui_internal.h"
#include "inventory_p.h"
#include "skins.h"
#include "interfaces.h"
#include "i_panorama.h"
#include "widgets.h"


ImU32 GetRarityColor(int rarity) {
    switch (rarity) {
    case 0: // IR_DEFAULT - consumer grade
        return IM_COL32(200, 200, 200, 255); // light gray
    case 1: // IR_COMMON - industrial grade
        return IM_COL32(255, 255, 255, 255); // WHITE
    case 2: // IR_UNCOMMON - mil-spec grade
        return IM_COL32(100, 150, 255, 255); // light blue
    case 3: // IR_RARE - restricted
        return IM_COL32(0, 0, 150, 255); // dark blue
    case 4: // IR_MYTHICAL - classified
        return IM_COL32(150, 0, 150, 255); // purple
    case 5: // IR_LEGENDARY - covert
        return IM_COL32(255, 0, 150, 255); // bright pink
    case 6: // IR_ANCIENT - extremely rare item
        return IM_COL32(255, 0, 0, 255); // RED
    case 7: // IR_IMMORTAL - contraband
        return IM_COL32(255, 0, 0, 255); // RED
    default:
        return IM_COL32(128, 128, 128, 255); // default gray
    }
}

ImU32 GetColorWithAlpha(ImU32 color) {
    ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);
    colorVec.w *= SDK::menuAlpha; 
    return ImGui::ColorConvertFloat4ToU32(colorVec);
}

ImVec4 ImLerpVec4(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}
namespace W {
    void DrawLogo()
    {
        float boxSize = 25.0f; 
        ImVec2 pos = ImGui::GetCursorScreenPos(); 
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(
            ImVec2(pos.x + 5, pos.y),
            ImVec2(pos.x + 5 + boxSize, pos.y + boxSize),
            GetColorWithAlpha(IM_COL32(255, 255, 255, 255)), 
            6.0f 
        );

        ImVec2 textSize = ImGui::CalcTextSize("B");
        ImVec2 textPos = ImVec2(
            pos.x + 5 + (boxSize - textSize.x) * 0.5f,
            pos.y + 0 + (boxSize - textSize.y) * 0.5f
        );
        draw_list->AddText(textPos, GetColorWithAlpha(IM_COL32(0, 0, 0, 255)), "B"); 

        ImGui::Dummy(ImVec2(boxSize, boxSize));
        ImGui::SameLine();
        ImGui::Text("ERSERK");
    }

    bool AnimatedTabButton(const char* label, int tab_index, bool is_active, ImVec2 size) {
        ImGui::PushID(tab_index);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        // Проверяем наведение и нажатие
        bool is_hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        bool is_pressed = is_hovered && ImGui::IsMouseDown(0);
        bool was_clicked = false;

        if (is_hovered && ImGui::IsMouseReleased(0)) {
            was_clicked = true;
        }

        float dt = ImGui::GetIO().DeltaTime;

        static std::map<int, TabAnimation> tabs;

        float target_hover = is_hovered ? 1.0f : 0.0f;
        tabs[tab_index].hover_alpha = ImLerp(tabs[tab_index].hover_alpha, target_hover, dt * 8.0f);

        float target_active = is_active ? 1.0f : 0.0f;
        tabs[tab_index].active_alpha = ImLerp(tabs[tab_index].active_alpha, target_active, dt * 6.0f);

        float target_scale = is_pressed ? 0.95f : 1.0f;
        tabs[tab_index].press_scale = ImLerp(tabs[tab_index].press_scale, target_scale, dt * 15.0f);

        ImU32 base_color = IM_COL32(15, 15, 15, 255);
        ImU32 hover_color = IM_COL32(60, 60, 65, 255);
        ImU32 active_color = IM_COL32(220, 50, 50, 255);

        ImVec4 bg_color_vec = ImGui::ColorConvertU32ToFloat4(base_color);

        if (tabs[tab_index].hover_alpha > 0.0f) {
            ImVec4 hover_vec = ImGui::ColorConvertU32ToFloat4(hover_color);
            bg_color_vec = ImLerpVec4(bg_color_vec, hover_vec, tabs[tab_index].hover_alpha);
        }

        if (tabs[tab_index].active_alpha > 0.0f) {
            ImVec4 active_vec = ImGui::ColorConvertU32ToFloat4(active_color);
            bg_color_vec = ImLerpVec4(bg_color_vec, active_vec, tabs[tab_index].active_alpha);
        }

        bg_color_vec.w *= SDK::menuAlpha;

        ImU32 final_bg_color = ImGui::ColorConvertFloat4ToU32(bg_color_vec);

        ImVec2 scaled_size = ImVec2(size.x * tabs[tab_index].press_scale, size.y * tabs[tab_index].press_scale);
        ImVec2 offset = ImVec2((size.x - scaled_size.x) * 0.5f, (size.y - scaled_size.y) * 0.5f);
        ImVec2 scaled_pos = ImVec2(pos.x + offset.x, pos.y + offset.y);

        draw_list->AddRectFilled(scaled_pos, ImVec2(scaled_pos.x + scaled_size.x, scaled_pos.y + scaled_size.y),
            final_bg_color, 6.0f);

        ImU32 border_color = is_active ? IM_COL32(255, 70, 70, 255) : IM_COL32(80, 80, 85, 255);
        float border_alpha = 0.7f + tabs[tab_index].hover_alpha * 0.3f + tabs[tab_index].active_alpha * 0.3f;
        ImVec4 border_vec = ImGui::ColorConvertU32ToFloat4(border_color);
        border_vec.w *= border_alpha * SDK::menuAlpha;
        draw_list->AddRect(scaled_pos, ImVec2(scaled_pos.x + scaled_size.x, scaled_pos.y + scaled_size.y),
            ImGui::ColorConvertFloat4ToU32(border_vec), 6.0f, 0, 1.5f);

        if (is_active && tabs[tab_index].active_alpha > 0.5f) {
            float glow_intensity = 0.4f + 0.3f * sinf(ImGui::GetTime() * 2.5f);
            ImU32 glow_color = IM_COL32(255, 100, 100, (int)(80 * glow_intensity * tabs[tab_index].active_alpha * SDK::menuAlpha));
            draw_list->AddRect(ImVec2(scaled_pos.x - 2, scaled_pos.y - 2),
                ImVec2(scaled_pos.x + scaled_size.x + 2, scaled_pos.y + scaled_size.y + 2),
                glow_color, 8.0f, 0, 2.0f);
        }

        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImVec2 text_pos = ImVec2(
            scaled_pos.x + (scaled_size.x - text_size.x) * 0.5f,
            scaled_pos.y + (scaled_size.y - text_size.y) * 0.5f
        );

        ImU32 text_color = IM_COL32(255, 255, 255, 255);
        if (is_active) {
            ImVec4 active_text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 red_tint = ImVec4(1.0f, 0.9f, 0.9f, 1.0f);
            ImVec4 final_text = ImLerpVec4(active_text, red_tint, tabs[tab_index].active_alpha * 0.3f);
            text_color = ImGui::ColorConvertFloat4ToU32(final_text);
        }

        ImVec4 text_vec = ImGui::ColorConvertU32ToFloat4(text_color);
        text_vec.w *= SDK::menuAlpha;
        text_color = ImGui::ColorConvertFloat4ToU32(text_vec);

        draw_list->AddText(text_pos, text_color, label);

        ImGui::InvisibleButton(("##tab_" + std::string(label)).c_str(), size);

        ImGui::PopID();
        return was_clicked;
    }

    bool AnimatedButton(const char* label, int button_index, ImVec2 size, bool is_tab_mode, bool* tab_state) {
        if (size.x <= 0 || size.y <= 0) {
            return false;
        }
        
        ImGui::PushID(button_index);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        bool is_hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        bool is_pressed = is_hovered && ImGui::IsMouseDown(0);
        bool was_clicked = false;

        if (is_hovered && ImGui::IsMouseReleased(0)) {
            was_clicked = true;
        }

        float dt = ImGui::GetIO().DeltaTime;

        static std::map<int, TabAnimation> button_animations;

        bool is_active = false;
        if (is_tab_mode && tab_state) {
            is_active = *tab_state;
        }

        float target_hover = is_hovered ? 1.0f : 0.0f;
        button_animations[button_index].hover_alpha = ImLerp(button_animations[button_index].hover_alpha, target_hover, dt * 8.0f);

        float target_active = (is_tab_mode && is_active) ? 1.0f : 0.0f;
        button_animations[button_index].active_alpha = ImLerp(button_animations[button_index].active_alpha, target_active, dt * 6.0f);

        float target_scale = is_pressed ? 0.95f : 1.0f;
        button_animations[button_index].press_scale = ImLerp(button_animations[button_index].press_scale, target_scale, dt * 15.0f);

        ImU32 base_color = IM_COL32(15, 15, 15, 255);
        ImU32 hover_color = IM_COL32(60, 60, 65, 255);
        ImU32 active_color = IM_COL32(220, 50, 50, 255); 

        ImVec4 bg_color_vec = ImGui::ColorConvertU32ToFloat4(base_color);

        if (button_animations[button_index].hover_alpha > 0.0f) {
            ImVec4 hover_vec = ImGui::ColorConvertU32ToFloat4(hover_color);
            bg_color_vec = ImLerpVec4(bg_color_vec, hover_vec, button_animations[button_index].hover_alpha);
        }

        if (button_animations[button_index].active_alpha > 0.0f) {
            ImVec4 active_vec = ImGui::ColorConvertU32ToFloat4(active_color);
            bg_color_vec = ImLerpVec4(bg_color_vec, active_vec, button_animations[button_index].active_alpha);
        }

        bg_color_vec.w *= SDK::menuAlpha;

        ImU32 final_bg_color = ImGui::ColorConvertFloat4ToU32(bg_color_vec);

        ImVec2 scaled_size = ImVec2(size.x * button_animations[button_index].press_scale, size.y * button_animations[button_index].press_scale);
        ImVec2 offset = ImVec2((size.x - scaled_size.x) * 0.5f, (size.y - scaled_size.y) * 0.5f);
        ImVec2 scaled_pos = ImVec2(pos.x + offset.x, pos.y + offset.y);

        draw_list->AddRectFilled(scaled_pos, ImVec2(scaled_pos.x + scaled_size.x, scaled_pos.y + scaled_size.y),
            final_bg_color, 6.0f);

        ImU32 border_color = (is_tab_mode && is_active) ? IM_COL32(255, 70, 70, 255) : IM_COL32(80, 80, 85, 255);
        float border_alpha = 0.7f + button_animations[button_index].hover_alpha * 0.3f + button_animations[button_index].active_alpha * 0.3f;
        ImVec4 border_vec = ImGui::ColorConvertU32ToFloat4(border_color);
        border_vec.w *= border_alpha * SDK::menuAlpha;
        draw_list->AddRect(scaled_pos, ImVec2(scaled_pos.x + scaled_size.x, scaled_pos.y + scaled_size.y),
            ImGui::ColorConvertFloat4ToU32(border_vec), 6.0f, 0, 1.5f);

        if (is_tab_mode && is_active && button_animations[button_index].active_alpha > 0.5f) {
            float glow_intensity = 0.4f + 0.3f * sinf(ImGui::GetTime() * 2.5f);
            ImU32 glow_color = IM_COL32(255, 100, 100, (int)(80 * glow_intensity * button_animations[button_index].active_alpha * SDK::menuAlpha));
            draw_list->AddRect(ImVec2(scaled_pos.x - 2, scaled_pos.y - 2),
                ImVec2(scaled_pos.x + scaled_size.x + 2, scaled_pos.y + scaled_size.y + 2),
                glow_color, 8.0f, 0, 2.0f);
        }

        ImVec2 text_size = ImGui::CalcTextSize(label);
        ImVec2 text_pos = ImVec2(
            scaled_pos.x + (scaled_size.x - text_size.x) * 0.5f,
            scaled_pos.y + (scaled_size.y - text_size.y) * 0.5f
        );

        // Цвет текста с анимацией
        ImU32 text_color = IM_COL32(255, 255, 255, 255);
        if (is_tab_mode && is_active) {
            ImVec4 active_text = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImVec4 red_tint = ImVec4(1.0f, 0.9f, 0.9f, 1.0f);
            ImVec4 final_text = ImLerpVec4(active_text, red_tint, button_animations[button_index].active_alpha * 0.3f);
            text_color = ImGui::ColorConvertFloat4ToU32(final_text);
        }

        // Применяем анимацию меню к альфа-каналу текста
        ImVec4 text_vec = ImGui::ColorConvertU32ToFloat4(text_color);
        text_vec.w *= SDK::menuAlpha;
        text_color = ImGui::ColorConvertFloat4ToU32(text_vec);

        draw_list->AddText(text_pos, text_color, label);

        // Резервируем место для кнопки
        ImGui::InvisibleButton(("##btn_" + std::string(label)).c_str(), size);

        ImGui::PopID();
        return was_clicked;
    }

    void RenderSkinManagerWindow() {
        static std::vector<PersistedEconItem> loadedItems;
        static bool itemsLoaded = false;
        static int selectedItemIndex = -1;
        static char searchFilter[256] = "";
        static bool showConfirmDelete = false;
        static bool showConfirmDeleteAll = false;
        static int itemToDelete = -1;

        // Загружаем скины при первом открытии
        if (!itemsLoaded) {
            loadedItems = InventoryPersistence::LoadJsonItems("added_items.json");
            itemsLoaded = true;
        }

        ImGui::BeginChild("SkinManagerContent", ImVec2(-1, -1), true);
        
        // Заголовок с кнопками управления
        ImGui::Text("Skin Manager");
        ImGui::Separator();
        ImGui::Spacing();

        // Кнопки управления
        if (W::AnimatedButton("Refresh", 100, ImVec2(100, 30), false, nullptr)) {
            loadedItems = InventoryPersistence::LoadJsonItems("added_items.json");
            selectedItemIndex = -1;
        }
        ImGui::SameLine();
        if (W::AnimatedButton("Clear All", 101, ImVec2(100, 30), false, nullptr)) {
            showConfirmDeleteAll = true;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Поиск
        ImGui::Text("Search:");
        ImGui::SameLine();
        ImGui::InputText("##SearchFilter", searchFilter, sizeof(searchFilter));
        ImGui::Spacing();

        // Информация о количестве скинов
        ImGui::Text("Total Skins: %d", (int)loadedItems.size());
        ImGui::Spacing();

        // Список скинов в плиточном виде
        ImGui::BeginChild("SkinList", ImVec2(0, 0), true);
        
        if (loadedItems.empty()) {
            ImGui::Text("No skins found in JSON file.");
            ImGui::Text("Add some skins first!");
        } else {
            // Фильтрация скинов
            std::vector<int> filteredIndices;
            for (int i = 0; i < loadedItems.size(); i++) {
                const auto& item = loadedItems[i];
                std::string searchTerm = searchFilter;
                std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), ::tolower);
                
                // Поиск по названию оружия, скина или defIndex
                std::string weaponName = GetLocalizedWeaponName(item.weaponTag);
                std::string skinName = GetLocalizedSkinName(item.skinTag);
                std::string itemStr = std::to_string(item.defIndex) + " " + weaponName + " " + skinName;
                std::transform(itemStr.begin(), itemStr.end(), itemStr.begin(), ::tolower);
                
                if (searchTerm.empty() || itemStr.find(searchTerm) != std::string::npos) {
                    filteredIndices.push_back(i);
                }
            }

            const float tileWidth = 192.f;
            const float tileHeight = 144.f;
            float spacing = ImGui::GetStyle().ItemSpacing.x; // отступы между плитками
            float windowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
            for (int idx : filteredIndices) {
                const auto& item = loadedItems[idx];
                
                void* skinImage = nullptr;
                CUIEngineSource2* pUIEngine = I::panorama->AccessUIEngine();
                CImageResourceManager* pResourceManager = pUIEngine ? pUIEngine->GetResourceManager() : nullptr;
                if (pResourceManager && !item.imagePath.empty()) {
                    skinImage = pResourceManager->LoadImageInternal(item.imagePath.c_str(), EImageFormat::RGBA8888);
                }
                
                // Получаем локализованные названия
                std::string weaponName = GetLocalizedWeaponName(item.weaponTag);
                std::string skinName = GetLocalizedSkinName(item.skinTag);
                
                // Создаем отображаемое название
                std::string displayName = weaponName;
                if (!skinName.empty() && skinName != "Unknown Skin") {
                    displayName += "\n" + skinName;
                }
                
                // Добавляем флаги
            
                // Проверяем, что строка не пустая
                if (displayName.empty()) {
                    displayName = "Unknown Item";
                }

                
                ImGui::PushID(idx);
                

                ImVec2 tileSize(tileWidth, tileHeight);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                
                // Отрисовываем плитку
                bool isSelected = (selectedItemIndex == idx);
                if (DrawSkinTile(displayName.c_str(), ImVec2(tileWidth, tileHeight), item.rarity, isSelected)) {
                    selectedItemIndex = idx;
                    itemToDelete = idx;
                    showConfirmDelete = true;
                }
                
                // Отрисовываем изображение скина после плитки (как в menu.cpp)
                if (skinImage) {
                    CImageProxySource* imageProxy = (CImageProxySource*)skinImage;
                    if (imageProxy) {
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        ImVec2 min = pos;
                        ImVec2 max = ImVec2(pos.x + tileWidth, pos.y + tileHeight);
                        ImVec2 pad = ImGui::GetStyle().FramePadding;
                        float textAreaH = ImGui::CalcTextSize("A").y + pad.y * 2.0f;
                        ImVec2 iconAreaMin = ImVec2(min.x + pad.x, min.y + pad.y);
                        ImVec2 iconAreaMax = ImVec2(max.x - pad.x, max.y - textAreaH - pad.y);

                        ImVec2 imgSize = imageProxy->GetImageSize();
                        float availW = iconAreaMax.x - iconAreaMin.x;
                        float availH = iconAreaMax.y - iconAreaMin.y;
                        float scale = 1.0f;
                        if (imgSize.x > 0.f && imgSize.y > 0.f) {
                            float scaleW = availW / imgSize.x;
                            float scaleH = availH / imgSize.y;
                            scale = (scaleW < scaleH ? scaleW : scaleH);
                        }
                        ImVec2 drawSize = ImVec2(imgSize.x * scale, imgSize.y * scale);
                        ImVec2 center = ImVec2((iconAreaMin.x + iconAreaMax.x) * 0.5f, (iconAreaMin.y + iconAreaMax.y) * 0.5f);
                        float offsetY = 8.0f; // сколько пикселей вниз опустить
                        ImVec2 imgMin = ImVec2(center.x - drawSize.x * 0.5f, center.y - drawSize.y * 0.5f + offsetY);
                        ImVec2 imgMax = ImVec2(center.x + drawSize.x * 0.5f, center.y + drawSize.y * 0.5f + offsetY);


                        dl->AddImageRounded((ImTextureID)imageProxy->GetNativeTexture(), imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), GetColorWithAlpha(IM_COL32_WHITE), 2.0f, ImDrawFlags_RoundCornersAll);
                    }
                }
                
             /*   ImVec2 tilePos = ImGui::GetItemRectMin();
                ImVec2 deleteBtnSize = ImVec2(20, 20);
                ImVec2 deleteBtnPos = ImVec2(tilePos.x + tileWidth - deleteBtnSize.x - 4, tilePos.y + 4);
                
                ImGui::SetCursorScreenPos(deleteBtnPos);
                if (W::AnimatedButton("X", 200 + idx, deleteBtnSize, false, nullptr)) {
                    itemToDelete = idx;
                    showConfirmDelete = true;
                }*/
                // перенос курсора на следующую плитку
                float lastX2 = ImGui::GetItemRectMax().x;
                float nextX2 = lastX2 + spacing + tileSize.x;
                if (nextX2 < windowVisibleX2) {
                    ImGui::SameLine();
                }
                ImGui::PopID();

            }
        }
        
        ImGui::EndChild();


        // Модальные окна подтверждения
        if (showConfirmDelete && itemToDelete >= 0 && !loadedItems.empty()) {
            ImGui::OpenPopup("Confirm Delete");
        }
        
        if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete this skin?");
            if (itemToDelete >= 0 && itemToDelete < loadedItems.size()) {
                const auto& item = loadedItems[itemToDelete];
                std::string weaponName = GetLocalizedWeaponName(item.weaponTag);
                std::string skinName = GetLocalizedSkinName(item.skinTag);
                
                ImGui::Text("Weapon: %s", weaponName.c_str());
                if (!skinName.empty() && skinName != "Unknown Skin") {
                    ImGui::Text("Skin: %s", skinName.c_str());
                }
                ImGui::Text("DefIndex: %d", item.defIndex);
            } else {
                ImGui::Text("Invalid item selected");
            }
            ImGui::Spacing();
            
            if (ImGui::Button("Yes", ImVec2(80, 0))) {
                // Проверяем валидность индекса перед удалением
                if (itemToDelete >= 0 && itemToDelete < loadedItems.size()) {
                    const auto& item = loadedItems[itemToDelete];
                    // Удаляем элемент из вектора
                    loadedItems.erase(loadedItems.begin() + itemToDelete);
                    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
                    if (!pInventory) return;
                    pInventory->RemoveEconItem(pInventory->GetSOCDataForItem(item.itemID));
                    // Сохраняем обновленный список в JSON
                    InventoryPersistence::SaveJsonItems(loadedItems, "added_items.json");
                    
                    // Сбрасываем выбор
                    if (selectedItemIndex >= itemToDelete) {
                        selectedItemIndex = std::max(0, selectedItemIndex - 1);
                    }
                    if (selectedItemIndex >= loadedItems.size()) {
                        selectedItemIndex = -1;
                    }
                }
                
                showConfirmDelete = false;
                itemToDelete = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(80, 0))) {
                showConfirmDelete = false;
                itemToDelete = -1;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (showConfirmDeleteAll) {
            ImGui::OpenPopup("Confirm Delete All");
        }
        
        if (ImGui::BeginPopupModal("Confirm Delete All", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Are you sure you want to delete ALL skins?");
            ImGui::Text("This action cannot be undone!");
            ImGui::Spacing();
            
            if (ImGui::Button("Yes, Delete All", ImVec2(120, 0))) {
                InventoryPersistence::ClearAllSkins();
                loadedItems.clear();
                selectedItemIndex = -1;
                showConfirmDeleteAll = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80, 0))) {
                showConfirmDeleteAll = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::EndChild();
    }

    std::string GetLocalizedWeaponName(const std::string& weaponTag) {
        if (weaponTag.empty() || !I::localize) {
            return "Unknown Weapon";
        }
        
        std::string localized = I::localize->find_key(weaponTag.c_str());
        if (localized.empty() || localized[0] == '#') {
            return weaponTag; // Return tag if localization failed
        }
        return localized;
    }

    std::string GetLocalizedSkinName(const std::string& skinTag) {
        if (skinTag.empty() || !I::localize) {
            return "Unknown Skin";
        }
        
        std::string localized = I::localize->find_key(skinTag.c_str());
        if (localized.empty() || localized[0] == '#') {
            return skinTag; // Return tag if localization failed
        }
        return localized;
    }

    std::string GetSkinImagePath(const std::string& weaponTag, const std::string& skinToken) {
        if (weaponTag.empty()) {
            return "";
        }
        
        // Check if this is an agent (customplayer_*)
        if (weaponTag.find("customplayer_") == 0) {
            // Agent image path
            std::string path = "s2r://panorama/images/econ/characters/";
            path += weaponTag;
            path += "_png.vtex";
            return path;
        }
        
        // For weapons, skins, gloves, etc. - need skinToken
        if (skinToken.empty()) {
            return "";
        }
        
        // Extract weapon name from tag (e.g., "weapon_ak47" -> "ak47")
        std::string weaponName = weaponTag;
        
        // Build image path similar to inventory.cpp
        std::string path = "s2r://panorama/images/econ/default_generated/";
        path += weaponName;
        path += "_";
        path += skinToken;
        path += "_light_png.vtex";
        
        return path;
    }

    std::string GetRarityName(int rarity) {
        switch (rarity) {
            case 0: return "Consumer Grade";
            case 1: return "Industrial Grade";
            case 2: return "Mil-Spec Grade";
            case 3: return "Restricted";
            case 4: return "Classified";
            case 5: return "Covert";
            case 6: return "Ancient";
            case 7: return "Contraband";
            default: return "Unknown";
        }
    }

    bool DrawSkinTile(const char* text, const ImVec2& size, int rarity, bool isSelected) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        // Use invisible button for hover/click
        bool pressed = false;
        if (ImGui::InvisibleButton("##skin_tile", size))
            pressed = true;

        const bool hovered = ImGui::IsItemHovered();
        const bool active = ImGui::IsItemActive();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 min = pos;
        ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

        // Base button color
        ImU32 col = ImGui::GetColorU32(ImGuiCol_Button);
        if (hovered) col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        if (active) col = IM_COL32(255, 255, 255, 200);

        float rounding = ImGui::GetStyle().FrameRounding;

        // Dark gradient background with hover effect
        ImU32 darkTop, darkBottom;
        if (isSelected) {
            // Selected state - brighter
            darkTop = GetColorWithAlpha(IM_COL32(60, 60, 70, 255));
            darkBottom = GetColorWithAlpha(IM_COL32(50, 50, 60, 255));
        } else if (hovered) {
            // Brighter gradient on hover
            darkTop = GetColorWithAlpha(IM_COL32(45, 45, 50, 255));
            darkBottom = GetColorWithAlpha(IM_COL32(35, 35, 40, 255));
        } else {
            // Normal dark gradient
            darkTop = GetColorWithAlpha(IM_COL32(30, 30, 35, 255));
            darkBottom = GetColorWithAlpha(IM_COL32(20, 20, 25, 255));
        }
        dl->AddRectFilledMultiColor(
            min, max,
            darkTop, darkTop,     // top corners
            darkBottom, darkBottom // bottom corners
        );

        // Left rarity color stripe (4px width) with hover effect
        if (rarity >= 0) {
            ImU32 rarityColor = GetRarityColor(rarity);
            if (hovered || isSelected) {
                // Make rarity stripe brighter on hover/selection
                ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(rarityColor);
                colorVec.w = 1.0f; // Ensure alpha = 1.0
                // Increase brightness by 20%
                colorVec.x = std::min(1.0f, colorVec.x * 1.2f);
                colorVec.y = std::min(1.0f, colorVec.y * 1.2f);
                colorVec.z = std::min(1.0f, colorVec.z * 1.2f);
                rarityColor = ImGui::ColorConvertFloat4ToU32(colorVec);
            }
            rarityColor = GetColorWithAlpha(rarityColor);
            ImVec2 stripeMin = ImVec2(min.x, min.y);
            ImVec2 stripeMax = ImVec2(min.x + 4.0f, max.y);
            dl->AddRectFilled(stripeMin, stripeMax, rarityColor, 0.0f);
        }

        // Border with hover effect
        ImU32 borderColor;
        if (isSelected) {
            // Selected border - bright
            borderColor = GetColorWithAlpha(IM_COL32(255, 255, 255, 200));
        } else if (hovered) {
            // Brighter border on hover
            borderColor = GetColorWithAlpha(IM_COL32(80, 80, 90, 255));
        } else {
            // Normal border
            borderColor = GetColorWithAlpha(IM_COL32(60, 60, 65, 255));
        }
        dl->AddRect(min, max, borderColor, rounding);


        // Bottom text with clipping
        ImVec2 pad = ImGui::GetStyle().FramePadding;
        ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false);
        float maxTextWidth = size.x - pad.x * 2.0f;
        float drawTextWidth = (textSize.x > maxTextWidth) ? maxTextWidth : textSize.x;
        float textX = min.x + (size.x - drawTextWidth) * 0.5f;
        float textY = max.y - pad.y - textSize.y;

        // Text: unified white with black outline, with hover effect
        ImU32 textColor, outline;
        if (isSelected) {
            // Selected text - bright
            textColor = GetColorWithAlpha(IM_COL32(255, 255, 180, 255));
            outline = GetColorWithAlpha(IM_COL32(0, 0, 0, 240));
        } else if (hovered) {
            // Brighter text on hover
            textColor = GetColorWithAlpha(IM_COL32(255, 255, 200, 255)); // slightly yellowish tint
            outline = GetColorWithAlpha(IM_COL32(0, 0, 0, 220)); // more contrasting outline
        } else {
            textColor = GetColorWithAlpha(IM_COL32(255, 255, 255, 255));
            outline = GetColorWithAlpha(IM_COL32(0, 0, 0, 200));
        }

        dl->PushClipRect(ImVec2(min.x + pad.x, min.y), ImVec2(max.x - pad.x, max.y), true);



        dl->PopClipRect();

        return pressed;
    }

}