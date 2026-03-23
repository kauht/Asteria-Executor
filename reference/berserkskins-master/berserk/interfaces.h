#pragma once

#include "i_localize.h"
#include "i_file_system.h"
#include "i_engine_client.h"
#include "i_client.h"
#include "i_panorama.h"
#include "i_schema.h"
#include "i_game_resource.h"
#include "i_event_manager.h"
#include "i_pvs.h"
namespace I {
    //inline i_csgo_input* input = nullptr;
    //inline i_game_resource* gameresource = nullptr;
    inline i_engine_client* engine = nullptr;
    //inline CPVS* PVS = nullptr;
    //inline i_schemasystem* schema = nullptr;
    inline i_client* iclient = nullptr;
    inline i_localize* localize = nullptr;
    inline i_file_system* file_system = nullptr;
    inline i_event_manager* eventmanager = nullptr;
    inline void* inputsystem = nullptr;
    inline i_schemasystem* schema = nullptr;
    inline i_game_resource* gameresource = nullptr;
	inline i_pvs* pvs = nullptr;
	inline i_panorama* panorama = nullptr;
	bool initialize();
}