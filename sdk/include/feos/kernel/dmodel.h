#pragma once
#define FEOS_DMODEL_INCLUDED

#ifndef __cplusplus
#error "This header file requires C++."
#endif

#include "feos.h"
#include "../devcmd.h"
#include "dmodel/device.h"
#include "dmodel/fsroot.h"
#include "dmodel/dimpl.h"
#include "dmodel/stream.h"

typedef struct tag_devCookie* devCookie;
typedef struct tag_fsrootCookie* fsrootCookie;

struct DriverFuncs
{
	int (* Attach)();
	bool (* CanUnload)();
	void (* Detach)();
};

#ifdef FEOS_KMODULE
#define FEOS_EXPORTMODULE(symName) extern "C" const DriverFuncs __driverFuncs =
#else
#define FEOS_EXPORTMODULE(symName) extern "C" const DriverFuncs symName =
#endif

extern "C"
{
	devCookie DevRegister(KDevice* dev, const char* baseName = nullptr);
	void DevUnregister(devCookie cookie);

	KDevice* DevGet(const char* baseName, int id);

	fsrootCookie FSRootRegister(IFSRoot* fsroot, const char* fsrootName);
	void FSRootUnregister(IFSRoot* fsroot);
}
