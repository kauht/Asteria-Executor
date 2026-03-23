#include "imgui/imgui.h"

#include "draw.h"
#include "GUI.h"

#include "sdk.h"

namespace DRAW {

	void render()
	{
        if (SDK::showMenu || SDK::menuAlpha > 0.0f) {
			GUI::menu();
        }
	}
}