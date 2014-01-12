#pragma once
#pragma interface
#ifndef FEOS_DMODEL_INCLUDED
#error "You must include <dmodel.h> first!"
#endif

enum
{
	DevType_Null = 0,
	DevType_Stream,
	DevType_Block,
	DevType_Framebuffer,
	DevType_Network,

	DevType_User = 1024,
};

struct KDevice
{
	virtual int GetType() = 0;
	virtual word_t AddRef() = 0;
	virtual word_t Release() = 0;

	// Main device command method.
	// - cmdId: ID of the command to execute. See devcmd.h for a list of cmd IDs.
	// - arg1,arg2: meaning depends on the command ID.
	// Returns: >=0 on success (meaning depends on the command ID), negated error code on failure.
	virtual intptr_t Command(int cmdId, intptr_t arg1, intptr_t arg2) = 0;
};
