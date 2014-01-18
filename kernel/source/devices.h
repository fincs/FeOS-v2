#pragma once
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// For use by DevInitPlatform
#define LINK_MODULE(_name) do \
	{ \
		extern const DriverFuncs _name; \
		_name.Attach(); \
	} while (0)

void DevInit();
void DevInitPlatform(); // implemented by HAL

void IoInit();

#ifdef __cplusplus
}
#endif
