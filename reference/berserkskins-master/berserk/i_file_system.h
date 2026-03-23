#pragma once
#include "memory.h"
class i_file_system
{
public:
    bool exists(const char* file_name, const char* a3)
    {
        return M::call_virtual<bool>(this, 21, file_name, a3);
    }
};
