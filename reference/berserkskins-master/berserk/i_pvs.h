#pragma once

// used: MEM::CallVFunc
#include "memory.h"

class i_pvs
{
public:
	void Set(bool bState)
	{
		M::call_virtual<void*>(this, 6, bState);
	}
};
