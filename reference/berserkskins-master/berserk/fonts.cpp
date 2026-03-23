#include "fonts.h"
#include "fa-900.h"
#include "gunicon.h"
#include "fa.h"
#include <stdio.h>



namespace FONTS {
    ImFont* Regular = nullptr; // Main font for text (weapon names, player names) 
    ImFont* Bold = nullptr; // Bold font for accents (optional)
    // Icon fonts
// Icon fonts
    ImFont* Icons = nullptr;  // FontAwesome for UI icons
    ImFont* GunIcon = nullptr; // Font for weapon icons
    const char* WeaponToFontLetter(const char* weapon) {

        if (weapon == nullptr)
            return "";

        if (strcmp(weapon, "knife_ct") == 0) return "]";
        if (strcmp(weapon, "knife_t") == 0) return "[";

        if (strcmp(weapon, "deagle") == 0) return "A";
        if (strcmp(weapon, "elite") == 0) return "B";
        if (strcmp(weapon, "fiveseven") == 0) return "C";
        if (strcmp(weapon, "glock") == 0) return "D";
        if (strcmp(weapon, "revolver") == 0) return "J";
        if (strcmp(weapon, "hkp2000") == 0) return "E";
        if (strcmp(weapon, "p250") == 0) return "F";
        if (strcmp(weapon, "usp_silencer") == 0) return "G";
        if (strcmp(weapon, "tec9") == 0) return "H";
        if (strcmp(weapon, "cz75a") == 0) return "I";

        if (strcmp(weapon, "mac10") == 0) return "K";
        if (strcmp(weapon, "ump45") == 0) return "L";
        if (strcmp(weapon, "bizon") == 0) return "M";
        if (strcmp(weapon, "mp7") == 0) return "N";
        if (strcmp(weapon, "mp9") == 0) return "R";
        if (strcmp(weapon, "p90") == 0) return "O";

        if (strcmp(weapon, "galilar") == 0) return "Q";
        if (strcmp(weapon, "famas") == 0) return "R";
        if (strcmp(weapon, "m4a1_silencer") == 0) return "T";
        if (strcmp(weapon, "m4a1") == 0) return "S";
        if (strcmp(weapon, "aug") == 0) return "U";
        if (strcmp(weapon, "sg556") == 0) return "V";
        if (strcmp(weapon, "ak47") == 0) return "W";

        if (strcmp(weapon, "g3sg1") == 0) return "X";
        if (strcmp(weapon, "scar20") == 0) return "Y";
        if (strcmp(weapon, "awp") == 0) return "Z";
        if (strcmp(weapon, "ssg08") == 0) return "a";

        if (strcmp(weapon, "xm1014") == 0) return "b";
        if (strcmp(weapon, "sawedoff") == 0) return "c";
        if (strcmp(weapon, "mag7") == 0) return "d";
        if (strcmp(weapon, "nova") == 0) return "e";

        if (strcmp(weapon, "negev") == 0) return "f";
        if (strcmp(weapon, "m249") == 0) return "g";

        if (strcmp(weapon, "taser") == 0) return "h";

        if (strcmp(weapon, "flashbang") == 0) return "i";
        if (strcmp(weapon, "hegrenade") == 0) return "j";
        if (strcmp(weapon, "smokegrenade") == 0) return "k";
        if (strcmp(weapon, "molotov") == 0) return "l";
        if (strcmp(weapon, "decoy") == 0) return "m";
        if (strcmp(weapon, "incgrenade") == 0) return "n";

        if (strcmp(weapon, "c4") == 0) return "o";

        return "";
    }
    void Initialize(ImGuiIO& io) {
        // Common font settings с улучшенным сглаживанием
        ImFontConfig fontConfig;
        fontConfig.OversampleH = 4;        // Увеличиваем горизонтальное сглаживание
        fontConfig.OversampleV = 2;        // Увеличиваем вертикальное сглаживание
        fontConfig.PixelSnapH = false;     // Отключаем привязку к пикселям для плавности
        fontConfig.FontDataOwnedByAtlas = false;
        fontConfig.RasterizerMultiply = 1.2f; // Увеличиваем яркость для лучшей читаемости

        // Main font (Verdana)
        Regular = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 18.0f, &fontConfig, io.Fonts->GetGlyphRangesCyrillic());
        if (!Regular) {
            Regular = io.Fonts->AddFontDefault();
            printf("[FONTS] Warning: Failed to load Verdana, using default font\n");
        }

        // Bold font (Verdana Bold) с теми же настройками сглаживания
        fontConfig.MergeMode = false;
        Bold = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdanab.ttf", 18.0f, &fontConfig, io.Fonts->GetGlyphRangesCyrillic());
        if (!Bold) {
            Bold = Regular;
            printf("[FONTS] Warning: Failed to load Verdana-Bold, using Regular\n");
        }

        //// FontAwesome icons (merge with main font)
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.OversampleH = 2;
        iconsConfig.OversampleV = 1;
        iconsConfig.FontDataOwnedByAtlas = false; // 🔥 запрещаем ImGui освобождать память

        static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

        Icons = io.Fonts->AddFontFromMemoryTTF(
            (void*)fa_solid_900,
            sizeof(fa_solid_900),
            18.0f,
            &iconsConfig,
            iconsRanges
        );




    }


    bool IsInitialized() {
        return Regular != nullptr && Bold != nullptr && Icons != nullptr && GunIcon != nullptr;
    }
}