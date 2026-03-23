#pragma once
namespace SDK {
    inline bool showMenu = false;
    inline bool shouldUnload = false;
	inline int g_ScreenWidth = 0;
	inline int g_ScreenHeight = 0;
    
    // Menu animation variables
    inline float menuAlpha = 0.0f;
    inline float menuAlphaTarget = 0.0f;
    inline float menuAnimationSpeed = 8.0f;
    
    // Menu state variables
    inline bool showSkinSearch = true;  // Default to skin search
    inline bool showSettings = false;
    inline bool showSkinManager = false;


	inline bool cursorlaststate = false;
}