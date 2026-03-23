#pragma once
#include "memory.h"
#include <string>

class i_engine_client {
public:
    bool is_in_game() {
        return M::CallVFunc<bool, 38>(this);
    }

    bool is_connected() {
        return M::CallVFunc<bool, 39>(this);

    }

    void ExecuteClientCmd(std::string szCommand) {
        return M::CallVFunc<void, 49>(this, 0, szCommand.c_str(), 0x7FFEF001);
    }

    int get_local_player_index() {
        int idx = -1;
        M::CallVFunc<void, 53>(this, &idx, 0);
        return idx + 1;
    }
    const char* GetLevelName()
    {
        return M::CallVFunc<const char*, 62>(this);
    }

    const char* GetLevelNameShort()
    {
        return M::CallVFunc<const char*, 63>(this);
    }
};
