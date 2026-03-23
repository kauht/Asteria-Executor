#pragma once
#include "memory.h"
#include <stdio.h>

class i_localize {
public:
	const char* find_key(const char* key) {
		return M::call_virtual<const char*>(this, 17, key, "");
	}
};