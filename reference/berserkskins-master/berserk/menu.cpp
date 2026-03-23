#define NOMINMAX
#include "gui.h"

#include "imgui/imgui.h"
#include "widgets.h"
#include "sdk.h"
#include <unordered_map>
#include <string>
#include "inventory.h"
#include "interfaces.h"
#include <algorithm>
#include <random>
#include "CCPlayerInventory.h"
#include "CEconItem.h"
#include "enums.h"
#include "fonts.h"
#include "skins.h"
#include "inventory_p.h"
static constexpr float windowWidth = 540.f;

static std::unordered_map<std::string, float> textAnimationOffsets;
static std::unordered_map<std::string, float> textAnimationSpeeds;


void RenderInventoryWindow();
void RenderSettingsWindow();

// Function to get color with current alpha


// Function to automatically add a skin without popup
void AddSkinDirectly(DumpedItem_t* item, DumpedSkin_t* skin) {
    if (!item || !skin) return;
    
    CEconItem* pItem = CEconItem::CreateInstance();
    if (pItem) {
        CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
        auto highestIDs = pInventory->GetHighestIDs();
        pItem->m_ulID = highestIDs.first + 1;
        pItem->m_unInventory =1 << 30;
        pItem->m_unAccountID = uint32_t(pInventory->GetOwner().m_id);
        pItem->m_unDefIndex = item->m_defIdx;
        if (item->m_unusualItem) pItem->m_nQuality = IQ_UNUSUAL;
        
        // For skins: use skin rarity, for non-skins: use R0
        pItem->m_nRarity = std::clamp(skin->m_rarity, 0, 7);
        
        pItem->SetPaintKit((float)skin->m_ID);
        pItem->SetPaintSeed(1.0f); // Default seed
        pItem->SetPaintWear(0.0f); // Default wear
        // No StatTrak for auto-added items
        
        if (pInventory->AddEconItem(pItem)) {
            S::AddEconItemToList(pItem, (float)skin->m_ID, 1.0f, 0.0f, skin->legacy);
            PersistedEconItem rec{};
            rec.itemID = pItem->m_ulID;
            rec.defIndex = item->m_defIdx;
            rec.quality = (item->m_unusualItem ? IQ_UNUSUAL : pItem->m_nQuality);
            rec.rarity = pItem->m_nRarity;
            rec.paintKit = (float)skin->m_ID;
            rec.paintSeed = 1.0f;
            rec.paintWear = 0.0f;
            rec.legacy = skin->legacy;
            rec.unusual = item->m_unusualItem;
            
            // Заполняем новые поля для локализации и изображений
            rec.weaponTag = item->m_simpleName; // Используем внутреннее имя оружия (например, "weapon_deagle")
            rec.skinTag = skin->m_name;   // Используем локализованное название скина как тег
            rec.skinToken = skin->m_tokenName; // Токен скина для изображения
            
            std::string weaponName = item->m_simpleName;
            if (item->isAgent) {
                rec.imagePath = "s2r://panorama/images/characters/";
                rec.imagePath += weaponName;
                rec.imagePath += "_png.vtex";
            }
            else {
                rec.imagePath = "s2r://panorama/images/econ/default_generated/";
                rec.imagePath += weaponName;
                rec.imagePath += "_";
                rec.imagePath += skin->m_tokenName;

                rec.imagePath += "_light_png.vtex";
            }

            
            InventoryPersistence::Append(rec);
        }
    }
}
#include "fa.h"
void GUI::menu() {
    ImGui::PushFont(FONTS::GetBold());
    ImGui::SetNextWindowSize(ImVec2(1060, 630), ImGuiCond_Once);
    
    // Don't show window if alpha is too low
    if (SDK::menuAlpha < 0.01f) {
        ImGui::PopFont();
        return;
    }
    
    // Apply global alpha to all ImGui elements
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, SDK::menuAlpha);
    
    // Set rounded corners style
    ImGuiStyle& style = ImGui::GetStyle();
    float oldRounding = style.WindowRounding;
    style.WindowRounding = 12.0f; // Rounded corners

    ImGui::Begin("gui_menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
    ImGui::Spacing();
    W::DrawLogo();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();
    // Animated tab buttons for skin search, skin manager and settings
    if (W::AnimatedButton(ICON_FA_PALETTE , 0, ImVec2(40, 40), true, &SDK::showSkinSearch)) {
        SDK::showSkinSearch = true;
        SDK::showSettings = false;
        SDK::showSkinManager = false;
    }
    ImGui::SameLine();
    if (W::AnimatedButton(ICON_FA_DATABASE, 1, ImVec2(40, 40), true, &SDK::showSkinManager)) {
        SDK::showSkinSearch = false;
        SDK::showSettings = false;
        SDK::showSkinManager = true;
    }
    ImGui::SameLine();
    if (W::AnimatedButton(ICON_FA_COG , 2, ImVec2(40, 40), true, &SDK::showSettings)) {
        SDK::showSkinSearch = false;
        SDK::showSettings = true;
        SDK::showSkinManager = false;
    }
    // Restore original rounding and pop alpha style
    style.WindowRounding = oldRounding;
    // Show appropriate window based on selection
    if (SDK::showSkinSearch) {
        RenderInventoryWindow();
    } else if (SDK::showSkinManager) {
        W::RenderSkinManagerWindow();
    } else if (SDK::showSettings) {
        RenderSettingsWindow();
    }
    ImGui::PopFont();
    ImGui::End();
    

    ImGui::PopStyleVar(); // Pop the alpha style var
}
static DumpedItem_t* pSelectedItem = nullptr;
struct SearchEntry
{
    DumpedItem_t* item;
    DumpedSkin_t* skin; // nullptr means entire item without specific skin
    std::string display; // "Weapon | Skin" or just item name
};
static std::vector<SearchEntry> g_searchIndex;
static size_t g_searchIndexVersion = 0; // use size as simple version marker
static std::vector<DumpedItem_t> vecDumpedItems;
static bool PassFilterCI(const char* text, const char* filter)
{
    if (!filter || !*filter) return true;
    if (!text) return false;
    std::string a(text), b(filter);
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char c) { return (char)tolower(c); });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char c) { return (char)tolower(c); });
    return a.find(b) != std::string::npos;
}
void DrawAnimatedText(ImDrawList* dl, const char* text, ImVec2 pos, ImU32 color, ImU32 outline, float maxWidth, const std::string& animationKey) {
    if (!text || !*text) return;

    ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false);

    // Если помещается, просто рисуем
    if (textSize.x <= maxWidth) {
        dl->AddText(ImVec2(pos.x + 1, pos.y), outline, text);
        dl->AddText(ImVec2(pos.x - 1, pos.y), outline, text);
        dl->AddText(ImVec2(pos.x, pos.y + 1), outline, text);
        dl->AddText(ImVec2(pos.x, pos.y - 1), outline, text);
        dl->AddText(pos, color, text);
        return;
    }

    // Инициализация анимации
    if (textAnimationOffsets.find(animationKey) == textAnimationOffsets.end()) {
        textAnimationOffsets[animationKey] = 0.0f;
        textAnimationSpeeds[animationKey] = 80.0f; // px/sec
    }

    float& offset = textAnimationOffsets[animationKey];
    float& speed = textAnimationSpeeds[animationKey];

    float deltaTime = ImGui::GetIO().DeltaTime;
    offset += speed * deltaTime;

    // 🔹 Добавляем пробел (gap) после текста
    float gap = 20.0f;
    float cycleLength = textSize.x + gap;

    // "Бесконечный цикл"
    offset = fmod(offset, cycleLength);

    float currentX = pos.x - offset;

    // Рисуем две копии текста с пробелом
    for (int i = 0; i < 2; i++) {
        ImVec2 textPos = ImVec2(currentX + i * cycleLength, pos.y);

        dl->AddText(ImVec2(textPos.x + 1, textPos.y), outline, text);
        dl->AddText(ImVec2(textPos.x - 1, textPos.y), outline, text);
        dl->AddText(ImVec2(textPos.x, textPos.y + 1), outline, text);
        dl->AddText(ImVec2(textPos.x, textPos.y - 1), outline, text);
        dl->AddText(textPos, color, text);
    }
}


static bool DrawTileButtonBottomText(const char* text, const ImVec2& size, int rarity = 0)
{
    ImVec2 pos = ImGui::GetCursorScreenPos();
    // Use invisible button for hover/click
    bool pressed = false;
    if (ImGui::InvisibleButton("##tile", size))
        pressed = true;

    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 min = pos;
    ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);

    // Base button color
    ImU32 col = ImGui::GetColorU32(ImGuiCol_Button);
    if (hovered) col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
    if (active) col = ImGui::GetColorU32(ImGuiCol_ButtonActive);

    float rounding = ImGui::GetStyle().FrameRounding;

    // Dark gradient background with hover effect
    ImU32 darkTop, darkBottom;
    if (hovered) {
        // Brighter gradient on hover
        darkTop = GetColorWithAlpha(IM_COL32(45, 45, 50, 255));
        darkBottom = GetColorWithAlpha(IM_COL32(35, 35, 40, 255));
    }
    else {
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
        if (hovered) {
            // Make rarity stripe brighter on hover
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
    if (hovered) {
        // Brighter border on hover
        borderColor = GetColorWithAlpha(IM_COL32(80, 80, 90, 255));
    }
    else {
        // Normal border
        borderColor = GetColorWithAlpha(IM_COL32(60, 60, 65, 255));
    }
    dl->AddRect(min, max, borderColor, rounding);

    // Debug: display rarity in top right corner
    //if (rarity >= 0) {
    //    char rarityText[16];
    //    snprintf(rarityText, sizeof(rarityText), "R%d", rarity);
    //    ImVec2 raritySize = ImGui::CalcTextSize(rarityText);
    //    ImVec2 rarityPos = ImVec2(max.x - raritySize.x - 4.0f, min.y + 4.0f);
    //    
    //    // Debug background
    //    ImVec2 debugBgMin = ImVec2(rarityPos.x - 2.0f, rarityPos.y - 1.0f);
    //    ImVec2 debugBgMax = ImVec2(rarityPos.x + raritySize.x + 2.0f, rarityPos.y + raritySize.y + 1.0f);
    //    dl->AddRectFilled(debugBgMin, debugBgMax, IM_COL32(0, 0, 0, 150), 2.0f);
    //    
    //    // Debug text
    //    dl->AddText(rarityPos, IM_COL32(255, 255, 255, 255), rarityText);
    //}

    // Bottom text with clipping
    ImVec2 pad = ImGui::GetStyle().FramePadding;
    ImVec2 textSize = ImGui::CalcTextSize(text, nullptr, false);
    float maxTextWidth = size.x - pad.x * 2.0f;
    float drawTextWidth = (textSize.x > maxTextWidth) ? maxTextWidth : textSize.x;
    float textX = min.x + (size.x - drawTextWidth) * 0.5f;
    float textY = max.y - pad.y - textSize.y;

    // Text: unified white with black outline, with hover effect
    ImU32 textColor, outline;
    if (hovered) {
        // Brighter text on hover
        textColor = GetColorWithAlpha(IM_COL32(255, 255, 200, 255)); // slightly yellowish tint
        outline = GetColorWithAlpha(IM_COL32(0, 0, 0, 220)); // more contrasting outline
    }
    else {
        textColor = GetColorWithAlpha(IM_COL32(255, 255, 255, 255));
        outline = GetColorWithAlpha(IM_COL32(0, 0, 0, 200));
    }

    dl->PushClipRect(ImVec2(min.x + pad.x, min.y), ImVec2(max.x - pad.x, max.y), true);

    // Create unique animation key based on text and position
    std::string animationKey = std::string(text) + "_" + std::to_string((int)min.x) + "_" + std::to_string((int)min.y);

    // Use animated text
    DrawAnimatedText(dl, text, ImVec2(textX, textY), textColor, outline, maxTextWidth, animationKey);

    dl->PopClipRect();

    return pressed;
}


static void RebuildSearchIndex()
{
    g_searchIndex.clear();
    for (auto& it : vecDumpedItems)
    {
        if (it.m_dumpedSkins.empty())
        {
            g_searchIndex.push_back({ &it, nullptr, it.m_name });
            continue;
        }
        for (auto& sk : it.m_dumpedSkins)
        {
            std::string disp = it.m_name;
            disp += " | ";
            disp += sk.m_name;
            g_searchIndex.push_back({ &it, &sk, std::move(disp) });
        }
    }
    g_searchIndexVersion = vecDumpedItems.size();
}

void RenderInventoryWindow() {
    static constexpr float windowWidth = 540.f;

    auto& vecDumpedItems = INVENTORY::GetDumpedItems();




    ImGui::Separator();

    // Search panel
    static char globalSearch[256] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##search", "Search item or skin", globalSearch, IM_ARRAYSIZE(globalSearch));

    ImGui::Separator();

    // View states: category selection → item list → skin list
    enum class Category { None = -1, Weapons, Gloves, Knives, Agents, Other };
    static Category selectedCategory = Category::None;
    static DumpedItem_t* activeBrowserItem = nullptr; // for showing skins of selected item

    // Two-panel layout
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float leftWidth = avail.x * 0.42f;
    float rightWidth = avail.x - leftWidth - 8.0f;

    // Add skin popup state
    static DumpedItem_t* popupItem = nullptr;
    static DumpedSkin_t* popupSkin = nullptr;
    static float popupWear = 0.f;
    static int popupSeed = 1;
    static int popupKills = -1;
    static char popupName[32] = {};
    static bool openAddSkinPopup = false; // open popup in parent ID context
    
    // Loading placeholder while inventory is being dumped
    if (!INVENTORY::IsDumped()) {
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x * 0.5f - 150, ImGui::GetWindowSize().y * 0.5f - 50));
        ImGui::BeginChild("##loading", ImVec2(290, 70), true, ImGuiWindowFlags_NoScrollbar);
        
        // Loading animation
        static float loadingTime = 0.0f;
        loadingTime += ImGui::GetIO().DeltaTime;
        float loadingProgress = fmod(loadingTime * 0.5f, 1.0f);
        
        ImGui::SetCursorPos(ImVec2(20, 20));
        ImGui::Text("Loading inventory...");
        
        ImGui::SetCursorPos(ImVec2(20, 45));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 255));     // фон - чёрный
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(255, 255, 255, 255)); // полоса - белая

        ImGui::ProgressBar(loadingProgress, ImVec2(260, 5), "");

        ImGui::PopStyleColor(2);
        
        ImGui::EndChild();
        return;
    }

    // Additional check to ensure we have items loaded
    if (vecDumpedItems.empty()) {
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x * 0.5f - 150, ImGui::GetWindowSize().y * 0.5f - 50));
        ImGui::BeginChild("##noitems", ImVec2(300, 100), true, ImGuiWindowFlags_NoScrollbar);
        
        ImGui::SetCursorPos(ImVec2(20, 20));
        ImGui::Text("No items found in inventory.");
        
        ImGui::SetCursorPos(ImVec2(20, 45));
        ImGui::Text("Please wait for inventory to load...");
        
        ImGui::EndChild();
        return;
    }

    if (g_searchIndexVersion != vecDumpedItems.size())
        RebuildSearchIndex();

    // Prepare UI engine for icons
    CUIEngineSource2* pUIEngine = I::panorama->AccessUIEngine();
    CImageResourceManager* pResourceManager = pUIEngine ? pUIEngine->GetResourceManager() : nullptr;
    // Left panel
    ImGui::BeginChild("##left", ImVec2(0, 0), true);
    // Main panel: category selection, then item list, then skin list
    {
        const float btnW = 192.f; // 4:3 aspect ratio
        const float btnH = 144.f;
        float maxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

        // Category selection screen (if no active search)
        if (selectedCategory == Category::None && !activeBrowserItem && (!globalSearch[0] || globalSearch[0] == '\0'))
        {
            // Category icons cache
            static CImageProxySource* catIcons[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
            if (pResourceManager)
            {
                if (!catIcons[0]) // Weapons - deagle
                    catIcons[0] = pResourceManager->LoadImageInternal("s2r://panorama/images/econ/weapons/base_weapons/weapon_deagle_png.vtex", EImageFormat::RGBA8888);
                if (!catIcons[1]) // Knives - butterfly
                    catIcons[1] = pResourceManager->LoadImageInternal("s2r://panorama/images/econ/weapons/base_weapons/weapon_knife_butterfly_png.vtex", EImageFormat::RGBA8888);
                if (!catIcons[2]) // Gloves - motorcycle
                    catIcons[2] = pResourceManager->LoadImageInternal("s2r://panorama/images/econ/default_generated/motorcycle_gloves_motorcycle_smoke_light_png.vtex", EImageFormat::RGBA8888);
                if (!catIcons[3]) // Agents - phoenix
                    catIcons[3] = pResourceManager->LoadImageInternal("s2r://panorama/images/econ/characters/customplayer_tm_phoenix_png.vtex", EImageFormat::RGBA8888);
                if (!catIcons[4]) // Other items - grenade
                    catIcons[4] = pResourceManager->LoadImageInternal("s2r://panorama/images/econ/weapons/base_weapons/weapon_hegrenade_png.vtex", EImageFormat::RGBA8888);
            }

            // Category tiles
            struct CatEntry { const char* title; Category cat; int iconIdx; };
            CatEntry cats[] = {
                { "Weapons", Category::Weapons, 0 },
                { "Knives", Category::Knives, 1 },
                { "Gloves", Category::Gloves, 2 },
                { "Agents", Category::Agents, 3 },
                { "Other", Category::Other, 4 }
            };
            for (auto& c : cats)
            {
                ImGui::PushID((int)c.cat);
                ImVec2 pos = ImGui::GetCursorScreenPos();
                bool pressed = DrawTileButtonBottomText(c.title, ImVec2(btnW, btnH), 0); // categories - consumer grade
                if (catIcons[c.iconIdx])
                {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 min = pos;
                    ImVec2 max = ImVec2(pos.x + btnW, pos.y + btnH);
                    ImVec2 pad = ImGui::GetStyle().FramePadding;
                    float textAreaH = ImGui::CalcTextSize("A").y + pad.y * 2.0f;
                    ImVec2 iconAreaMin = ImVec2(min.x + pad.x, min.y + pad.y);
                    ImVec2 iconAreaMax = ImVec2(max.x - pad.x, max.y - textAreaH - pad.y);

                    ImVec2 imgSize = catIcons[c.iconIdx]->GetImageSize();
                    float availW = iconAreaMax.x - iconAreaMin.x;
                    float availH = iconAreaMax.y - iconAreaMin.y;
                    float scale = 1.0f;
                    if (imgSize.x > 0.f && imgSize.y > 0.f)
                    {
                        float scaleW = availW / imgSize.x;
                        float scaleH = availH / imgSize.y;
                        scale = (scaleW < scaleH ? scaleW : scaleH);
                    }
                    ImVec2 drawSize = ImVec2(imgSize.x * scale, imgSize.y * scale);
                    ImVec2 center = ImVec2((iconAreaMin.x + iconAreaMax.x) * 0.5f, (iconAreaMin.y + iconAreaMax.y) * 0.5f);
                    ImVec2 imgMin = ImVec2(center.x - drawSize.x * 0.5f, center.y - drawSize.y * 0.5f);
                    ImVec2 imgMax = ImVec2(center.x + drawSize.x * 0.5f, center.y + drawSize.y * 0.5f);
                    dl->AddImageRounded((ImTextureID)catIcons[c.iconIdx]->GetNativeTexture(), imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), GetColorWithAlpha(IM_COL32_WHITE), 4.0f, ImDrawFlags_RoundCornersAll);
                }
                if (pressed)
                {
                    selectedCategory = c.cat;
                }
                ImGui::PopID();
                float nextX = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
                if (nextX + btnW <= maxX)
                    ImGui::SameLine();
            }
        }
        // Список предметов (если выбранная категория или активный поиск)
        else if (!activeBrowserItem)
        {
            // Кнопка Домой возвращает к выбору категорий
            if (ImGui::Button("Home"))
            {
                selectedCategory = Category::None;
                activeBrowserItem = nullptr;
            }
            // Собираем список отображаемых предметов и сортируем по редкости (убывание)
            std::vector<DumpedItem_t*> itemsView;
            itemsView.reserve(vecDumpedItems.size());
            
            // Глобальный поиск скинов - если есть поиск, показываем скины из всех предметов
            const bool hasSearch = (globalSearch[0] && globalSearch[0] != '\0');
            if (hasSearch) {
                // Собираем все скины, которые соответствуют поиску
                std::vector<std::pair<DumpedItem_t*, DumpedSkin_t*>> matchingSkins;
                for (auto& item : vecDumpedItems) {
                    for (auto& skin : item.m_dumpedSkins) {
                        if (PassFilterCI(skin.m_name.c_str(), globalSearch)) {
                            matchingSkins.push_back({&item, &skin});
                        }
                    }
                }
                
                // Сортируем скины по редкости
                std::sort(matchingSkins.begin(), matchingSkins.end(), 
                    [](const std::pair<DumpedItem_t*, DumpedSkin_t*>& a, 
                       const std::pair<DumpedItem_t*, DumpedSkin_t*>& b) { 
                        return a.second->m_rarity > b.second->m_rarity; 
                    });
                
                // Отображаем найденные скины
                for (const auto& skinPair : matchingSkins) {
                    DumpedItem_t* pItem = skinPair.first;
                    DumpedSkin_t* pSkin = skinPair.second;
                    
                    // Загружаем иконку скина если нужно
                    if (pResourceManager && pSkin->m_image == nullptr && !pItem->m_simpleName.empty() && !pSkin->m_tokenName.empty()) {
                        std::string path = "s2r://panorama/images/econ/default_generated/";
                        path += pItem->m_simpleName;
                        path += "_";
                        path += pSkin->m_tokenName;
                        path += "_light_png.vtex";
                        pSkin->m_image = pResourceManager->LoadImageInternal(path.c_str(), EImageFormat::RGBA8888);
                    }
                    
                    ImGui::PushID(pSkin);
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    std::string displayName = pItem->m_name + " | " + pSkin->m_name;
                    bool pressed = DrawTileButtonBottomText(displayName.c_str(), ImVec2(btnW, btnH), pSkin->m_rarity);
                    
                    if (pSkin->m_image) {
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        ImVec2 min = pos;
                        ImVec2 max = ImVec2(pos.x + btnW, pos.y + btnH);
                        ImVec2 pad = ImGui::GetStyle().FramePadding;
                        float textAreaH = ImGui::CalcTextSize("A").y + pad.y * 2.0f;
                        ImVec2 iconAreaMin = ImVec2(min.x + pad.x, min.y + pad.y);
                        ImVec2 iconAreaMax = ImVec2(max.x - pad.x, max.y - textAreaH - pad.y);

                        ImVec2 imgSize = pSkin->m_image->GetImageSize();
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
                        ImVec2 imgMin = ImVec2(center.x - drawSize.x * 0.5f, center.y - drawSize.y * 0.5f);
                        ImVec2 imgMax = ImVec2(center.x + drawSize.x * 0.5f, center.y + drawSize.y * 0.5f);
                        dl->AddImageRounded((ImTextureID)pSkin->m_image->GetNativeTexture(), imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), GetColorWithAlpha(IM_COL32_WHITE), 2.0f, ImDrawFlags_RoundCornersAll);
                    }
                    
                    if (pressed) {
                        pSelectedItem = pItem;
                        pSelectedItem->pSelectedSkin = pSkin;
                        popupItem = pSelectedItem;
                        popupSkin = pSkin;
                        popupWear = 0.f;
                        popupSeed = 1;
                        popupKills = -1;
                        memset(popupName, '\0', sizeof(popupName));
                        openAddSkinPopup = true;
                    }
                    
                    ImGui::PopID();
                    float nextX = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
                    if (nextX + btnW <= maxX)
                        ImGui::SameLine();
                }
            } else {
                // Обычная логика отображения предметов по категориям
                for (auto& item : vecDumpedItems)
                {
                    // Фильтрация по выбранной категории
                    bool include = false;
                    if (selectedCategory == Category::Weapons)
                        include = item.isWeapon && !item.isKnife && !item.isGloves && !item.isAgent;
                    else if (selectedCategory == Category::Knives)
                        include = item.isKnife;
                    else if (selectedCategory == Category::Gloves)
                        include = item.isGloves;
                    else if (selectedCategory == Category::Agents)
                        include = item.isAgent;
                    else if (selectedCategory == Category::Other)
                        include = item.isOther;
                    else
                        include = true;
                    
                    if (!include)
                        continue;
                    itemsView.push_back(&item);
                }
            }
            std::sort(itemsView.begin(), itemsView.end(), [](const DumpedItem_t* a, const DumpedItem_t* b) { return a->m_rarity > b->m_rarity; });

            for (DumpedItem_t* pItem : itemsView)
            {
                auto& item = *pItem;

                if (pResourceManager && item.m_image == nullptr && !item.m_simpleName.empty())
                {
                    std::string path;
                    if (item.isAgent)
                    {
                        path = "s2r://panorama/images/econ/characters/";
                        path += item.m_simpleName;
                        path += "_png.vtex";
                        item.m_image = pResourceManager->LoadImageInternal(path.c_str(), EImageFormat::RGBA8888);

                    }
                    else if (item.isGloves)
                    {
                        bool loaded = false;
                        if (!item.m_dumpedSkins.empty())
                        {
                            // Выбрать и сохранить случайный скин один раз
                            if (item.m_previewSkinIndex < 0)
                            {
                                std::mt19937 rng{ std::random_device{}() };
                                std::uniform_int_distribution<int> dist(0, (int)item.m_dumpedSkins.size() - 1);
                                item.m_previewSkinIndex = dist(rng);
                            }
                            const DumpedSkin_t& skin = item.m_dumpedSkins[item.m_previewSkinIndex];
                            if (!skin.m_tokenName.empty())
                            {
                                // s2r://panorama/images/econ/default_generated/<glove_simple>_<token>_light_png.vtex
                                path = "s2r://panorama/images/econ/default_generated/";
                                path += item.m_simpleName;
                                path += "_";
                                path += skin.m_tokenName;
                                path += "_light_png.vtex";
                                item.m_image = pResourceManager->LoadImageInternal(path.c_str(), EImageFormat::RGBA8888);

                                loaded = item.m_image != nullptr;
                            }
                        }
                    }
                    else if (item.isOther)
                    {
                        // Для других предметов используем базовую иконку гранаты
                        path = "s2r://panorama/images/econ/weapons/base_weapons/weapon_hegrenade_png.vtex";
                        item.m_image = pResourceManager->LoadImageInternal(path.c_str(), EImageFormat::RGBA8888);
                    }
                    else
                    {
                        path = "s2r://panorama/images/econ/weapons/base_weapons/";
                        path += item.m_simpleName;
                        path += "_png.vtex";
                        item.m_image = pResourceManager->LoadImageInternal(path.c_str(), EImageFormat::RGBA8888);
                    }
                }
                ImGui::PushID(&item);
                // Отрисовка плитки с иконкой
                ImVec2 pos = ImGui::GetCursorScreenPos();
                bool pressed = DrawTileButtonBottomText(item.m_name.c_str(), ImVec2(btnW, btnH), item.m_rarity);
                if (item.m_image)
                {
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec2 min = pos;
                    ImVec2 max = ImVec2(pos.x + btnW, pos.y + btnH);
                    // Зарезервируем область под подпись снизу
                    ImVec2 pad = ImGui::GetStyle().FramePadding;
                    float textAreaH = ImGui::CalcTextSize("A").y + pad.y * 2.0f;
                    ImVec2 iconAreaMin = ImVec2(min.x + pad.x, min.y + pad.y);
                    ImVec2 iconAreaMax = ImVec2(max.x - pad.x, max.y - textAreaH - pad.y);

                    ImVec2 imgSize = item.m_image->GetImageSize();
                    float availW = iconAreaMax.x - iconAreaMin.x;
                    float availH = iconAreaMax.y - iconAreaMin.y;
                    float scale = 1.0f;
                    if (imgSize.x > 0.f && imgSize.y > 0.f)
                    {
                        float scaleW = availW / imgSize.x;
                        float scaleH = availH / imgSize.y;
                        scale = (scaleW < scaleH ? scaleW : scaleH);
                    }
                    ImVec2 drawSize = ImVec2(imgSize.x * scale, imgSize.y * scale);
                    ImVec2 center = ImVec2((iconAreaMin.x + iconAreaMax.x) * 0.5f, (iconAreaMin.y + iconAreaMax.y) * 0.5f);
                    ImVec2 imgMin = ImVec2(center.x - drawSize.x * 0.5f, center.y - drawSize.y * 0.5f);
                    ImVec2 imgMax = ImVec2(center.x + drawSize.x * 0.5f, center.y + drawSize.y * 0.5f);
                    dl->AddImageRounded((ImTextureID)item.m_image->GetNativeTexture(), imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), GetColorWithAlpha(IM_COL32_WHITE), 2.0f, ImDrawFlags_RoundCornersAll);
                }
                if (pressed)
                {
                    // Агенты и Other: сразу добавляем скин без попапа
                    if (item.isAgent || item.isOther)
                    {
                        if (!item.m_dumpedSkins.empty()) {
                            AddSkinDirectly(&item, &item.m_dumpedSkins[0]);
                        }
                    }
                    else
                    {
                        activeBrowserItem = &item; // открыть список скинов этого предмета
                        pSelectedItem = &item; // синхронизируем правую панель
                    }
                }
                ImGui::PopID();
                float nextX = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
                if (nextX + btnW <= maxX)
                    ImGui::SameLine();
            }
        }
        // Список скинов выбранного предмета
        else
        {
            // Стабильный указатель на текущий предмет для этого кадра
            DumpedItem_t* currentItem = activeBrowserItem;
            if (!currentItem || currentItem->m_dumpedSkins.empty())
            {
                ImGui::Text("No skins for this item");
                if (ImGui::Button("Back"))
                {
                    activeBrowserItem = nullptr;
                }
            }
            else
            {
                // Кнопки навигации: Домой и Назад
                bool navigated = false;
                if (ImGui::Button("Home"))
                {
                    selectedCategory = Category::None;
                    activeBrowserItem = nullptr;
                    navigated = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Back"))
                {
                    activeBrowserItem = nullptr;
                    navigated = true;
                }
                if (navigated)
                {
                    // Не продолжаем рендер в этом фрейме, чтобы не трогать сброшенные указатели
                }
                else
                {
                    ImGui::Separator();
                    const float sBtnW = 192.f; // 4:3 пропорции
                    const float sBtnH = 144.f;
                    float sMaxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                    // Собираем и сортируем скины по редкости (убывание)
                    std::vector<DumpedSkin_t*> skinsView;
                    skinsView.reserve(currentItem->m_dumpedSkins.size());
                    for (auto& skin : currentItem->m_dumpedSkins)
                    {
                        if (!PassFilterCI(skin.m_name.c_str(), globalSearch))
                            continue;
                        skinsView.push_back(&skin);
                    }
                    std::sort(skinsView.begin(), skinsView.end(), [](const DumpedSkin_t* a, const DumpedSkin_t* b) { return a->m_rarity > b->m_rarity; });
                    bool stopFrame = false;
                    for (DumpedSkin_t* pSkin : skinsView)
                    {
                        auto& skin = *pSkin;
                        // Ленивая загрузка иконки скина с фоллбэком на .vtex_c
                        if (pResourceManager && skin.m_image == nullptr && !currentItem->m_simpleName.empty() && !skin.m_tokenName.empty())
                        {
                            // s2r://panorama/images/econ/default_generated/<weapon>_<token>_light_png.vtex
                            std::string path = "s2r://panorama/images/econ/default_generated/";
                            path += currentItem->m_simpleName;
                            path += "_";
                            path += skin.m_tokenName;
                            path += "_light_png.vtex";
                            skin.m_image = pResourceManager->LoadImageInternal(path.c_str(), EImageFormat::RGBA8888);
                        }

                        ImGui::PushID(&skin);
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        bool pressed = DrawTileButtonBottomText(skin.m_name.c_str(), ImVec2(sBtnW, sBtnH), skin.m_rarity);
                        if (skin.m_image)
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 min = pos;
                            ImVec2 max = ImVec2(pos.x + sBtnW, pos.y + sBtnH);
                            ImVec2 pad = ImGui::GetStyle().FramePadding;
                            float textAreaH = ImGui::CalcTextSize("A").y + pad.y * 2.0f;
                            ImVec2 iconAreaMin = ImVec2(min.x + pad.x, min.y + pad.y);
                            ImVec2 iconAreaMax = ImVec2(max.x - pad.x, max.y - textAreaH - pad.y);

                            ImVec2 imgSize = skin.m_image->GetImageSize();
                            float availW = iconAreaMax.x - iconAreaMin.x;
                            float availH = iconAreaMax.y - iconAreaMin.y;
                            float scale = 1.0f;
                            if (imgSize.x > 0.f && imgSize.y > 0.f)
                            {
                                float scaleW = availW / imgSize.x;
                                float scaleH = availH / imgSize.y;
                                scale = (scaleW < scaleH ? scaleW : scaleH);
                            }
                            ImVec2 drawSize = ImVec2(imgSize.x * scale, imgSize.y * scale);
                            ImVec2 center = ImVec2((iconAreaMin.x + iconAreaMax.x) * 0.5f, (iconAreaMin.y + iconAreaMax.y) * 0.5f);
                            ImVec2 imgMin = ImVec2(center.x - drawSize.x * 0.5f, center.y - drawSize.y * 0.5f);
                            ImVec2 imgMax = ImVec2(center.x + drawSize.x * 0.5f, center.y + drawSize.y * 0.5f);
                            dl->AddImageRounded((ImTextureID)skin.m_image->GetNativeTexture(), imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1), GetColorWithAlpha(IM_COL32_WHITE), 2.0f, ImDrawFlags_RoundCornersAll);
                        }
                        if (pressed)
                        {
                            pSelectedItem = currentItem;
                            pSelectedItem->pSelectedSkin = &skin;
                            popupItem = pSelectedItem;
                            popupSkin = &skin;
                            popupWear = 0.f;
                            popupSeed = 1;
                            popupKills = -1;
                            memset(popupName, '\0', sizeof(popupName));
                            openAddSkinPopup = true; // отложенное открытие в общем контексте
                            stopFrame = true;
                        }
                        ImGui::PopID();
                        float nextX = ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x;
                        if (nextX + sBtnW <= sMaxX)
                            ImGui::SameLine();
                        if (stopFrame) break;
                    }
                    if (stopFrame) { /* предотвращаем дальнейшие клики в этом фрейме */ }
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    //// Правая панель: лаконичная информация
    //ImGui::BeginChild("##right", ImVec2(rightWidth, avail.y), true);
    //if (pSelectedItem)
    //{
    //    ImGui::Text("Выбрано: %s", pSelectedItem->m_name.c_str());
    //    if (pSelectedItem->pSelectedSkin)
    //        ImGui::Text("Скин: %s", pSelectedItem->pSelectedSkin->m_name.c_str());
    //}
    //ImGui::EndChild();

    // Popup для подтверждения очистки JSON конфигов
    

    // Модальное окно добавления скина
    if (openAddSkinPopup)
    {
        ImGui::OpenPopup("addskin");
        openAddSkinPopup = false;
    }
    if (ImGui::BeginPopupModal("addskin", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (popupItem && popupSkin)
        {
            ImGui::Text("%s | %s", popupItem->m_name.c_str(), popupSkin->m_name.c_str());
            ImGui::Separator();
            ImGui::TextUnformatted("Wear Rating");
            ImGui::SliderFloat("##wear", &popupWear, 0.f, 1.f, "%.9f", ImGuiSliderFlags_Logarithmic);
            ImGui::TextUnformatted("Pattern Template");
            ImGui::SliderInt("##seed", &popupSeed, 1, 1000);
            ImGui::TextUnformatted("StatTrak Count");
            ImGui::SliderInt("##kills", &popupKills, -1, INT_MAX / 2, popupKills == -1 ? "Not StatTrak" : "%d", ImGuiSliderFlags_Logarithmic);
            ImGui::TextUnformatted("Custom Name");
            ImGui::InputTextWithHint("##cname", "Default", popupName, IM_ARRAYSIZE(popupName));

            if (ImGui::Button("Add", ImVec2(180, 0)))
            {
                CEconItem* pItem = CEconItem::CreateInstance();
                if (pItem) {
                    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
                    auto highestIDs = pInventory->GetHighestIDs();
                    pItem->m_ulID = highestIDs.first + 1;
                    pItem->m_unInventory = 1 << 30;
                    pItem->m_unAccountID = uint32_t(pInventory->GetOwner().m_id);
                    pItem->m_unDefIndex = popupItem->m_defIdx;
                    if (popupItem->m_unusualItem) pItem->m_nQuality = IQ_UNUSUAL;
                    // For skins: use skin rarity, for non-skins: use R0
                    if (popupSkin && popupSkin->m_ID > 0) {
                        pItem->m_nRarity = std::clamp(popupSkin->m_rarity, 0, 7);
                    }
                    else {
                        pItem->m_nRarity = 0; // R0 for non-skin items
                    }
                    pItem->SetPaintKit((float)popupSkin->m_ID);
                    pItem->SetPaintSeed((float)popupSeed);
                    pItem->SetPaintWear(popupWear);
                    if (popupKills >= 0) { pItem->SetStatTrak(popupKills); pItem->SetStatTrakType(0); if (pItem->m_nQuality != IQ_UNUSUAL) pItem->m_nQuality = IQ_STRANGE; }
                    if (pInventory->AddEconItem(pItem)) {
                        S::AddEconItemToList(pItem, (float)popupSkin->m_ID, (float)popupSeed, popupWear, popupSkin->legacy);
                        PersistedEconItem rec{};
                        rec.itemID = pItem->m_ulID;
                        rec.defIndex = popupItem->m_defIdx;
                        rec.quality = (popupItem->m_unusualItem ? IQ_UNUSUAL : pItem->m_nQuality);
                        rec.rarity = pItem->m_nRarity; // Already set correctly above
                        rec.paintKit = (float)popupSkin->m_ID;
                        rec.paintSeed = (float)popupSeed;
                        rec.paintWear = popupWear;
                        rec.legacy = popupSkin->legacy;
                        rec.unusual = popupItem->m_unusualItem;
                        rec.statTrak = popupKills; // Сохраняем StatTrak значение
                        
                        // Заполняем новые поля для локализации и изображений
                        rec.weaponTag = popupItem->m_simpleName; // Используем внутреннее имя оружия (например, "weapon_deagle")
                        rec.skinTag = popupSkin->m_name;   // Используем локализованное название скина как тег
                        rec.skinToken = popupSkin->m_tokenName; // Токен скина для изображения
                        // Создаем путь к изображению (как в menu.cpp)

                        rec.imagePath = "s2r://panorama/images/econ/default_generated/";

                        rec.imagePath += popupItem->m_simpleName;
                        rec.imagePath += "_";
                        rec.imagePath += popupSkin->m_tokenName;
                        rec.imagePath += "_light_png.vtex";
                        
                        InventoryPersistence::Append(rec);
                    }
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::TextUnformatted("No skin selected");
            if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

}
#include "hooks.h"
void RenderSettingsWindow() {
    ImGui::BeginChild("SettingsContent", ImVec2(0, 0), true);
    
    ImGui::Text("Settings");
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("Menu Animation Speed");
    ImGui::SliderFloat("##MenuSpeed", &SDK::menuAnimationSpeed, 1.0f, 20.0f, "%.1f");
        
    ImGui::Spacing();
    ImGui::Text("Menu Alpha: %.2f", SDK::menuAlpha);
    ImGui::ProgressBar(SDK::menuAlpha, ImVec2(-1, 0), "");
    
    ImGui::Spacing();
    


    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    if (W::AnimatedButton("Unload DLL", 20, ImVec2(200, 30), false, nullptr)) {
        SDK::shouldUnload = true;
        H::hkEnableCursor.GetOriginal()(I::inputsystem, SDK::cursorlaststate);

    }
    ImGui::SameLine();
    
    ImGui::EndChild();
}
