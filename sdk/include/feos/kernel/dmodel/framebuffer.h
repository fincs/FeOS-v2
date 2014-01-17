#pragma once
#ifndef FEOS_DMODEL_INCLUDED
#error "You must include <dmodel.h> first!"
#endif

struct KFramebuffer : public KDevice
{
	static inline KFramebuffer* From(KDevice* dev)
	{
		return (KFramebuffer*)dev;
	}

	inline intptr_t GetInfo(FbDevInfo& /*out*/ stat)
	{
		return Command(FbCmd_GetInfo, (intptr_t)&stat, 0);
	}

private:
	KFramebuffer();
	~KFramebuffer();
};

template <word_t& usageCount>
using KFramebufferImplMod = KDeviceImpl<DevType_Framebuffer, usageCount>;
using KFramebufferImpl = KFramebufferImplMod<g_defModUsageRefCnt>;

#define KFBCMDMAP_GetInfo(method) \
		case FbCmd_GetInfo: \
			return method(*((FbDevInfo*)arg1));
