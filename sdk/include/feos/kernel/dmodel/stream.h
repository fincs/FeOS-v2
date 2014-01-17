#pragma once
#ifndef FEOS_DMODEL_INCLUDED
#error "You must include <dmodel.h> first!"
#endif

struct KStream : public KDevice
{
	static inline KStream* From(KDevice* dev)
	{
		return (KStream*)dev;
	}

	inline intptr_t GetStat(StrmDevStat& /*out*/ stat)
	{
		return Command(StrmCmd_GetStat, (intptr_t)&stat, 0);
	}

	inline intptr_t Read(void* buffer, size_t bufSize)
	{
		return Command(StrmCmd_Read, (intptr_t)buffer, (intptr_t)bufSize);
	}

	inline intptr_t Write(const void* buffer, size_t bufSize)
	{
		return Command(StrmCmd_Write, (intptr_t)buffer, (intptr_t)bufSize);
	}

	inline intptr_t Seek(u64& /*in,out*/ pos, int mode)
	{
		return Command(StrmCmd_Seek, (intptr_t)&pos, (intptr_t)mode);
	}

	inline intptr_t Truncate(u64 pos)
	{
		return Command(StrmCmd_Truncate, (intptr_t)&pos, 0);
	}

	inline intptr_t Sync()
	{
		return Command(StrmCmd_Sync, 0, 0);
	}

private:
	KStream();
	~KStream();
};

template <word_t& usageCount>
using KStreamImplMod = KDeviceImpl<DevType_Stream, usageCount>;
using KStreamImpl = KStreamImplMod<g_defModUsageRefCnt>;

#define KSTRMCMDMAP_GetStat(method) \
		case StrmCmd_GetStat: \
			return method(*((StrmDevStat*)arg1));

#define KSTRMCMDMAP_Read(method) \
		case StrmCmd_Read: \
			return method((void*)arg1, (size_t)arg2);

#define KSTRMCMDMAP_Write(method) \
		case StrmCmd_Write: \
			return method((const void*)arg1, (size_t)arg2);

#define KSTRMCMDMAP_Seek(method) \
		case StrmCmd_Seek: \
			return method(*(u64*)arg1, (int)arg2);

#define KSTRMCMDMAP_Truncate(method) \
		case StrmCmd_Truncate: \
			return method(*(u64*)arg1);

#define KSTRMCMDMAP_Sync(method) \
		case StrmCmd_Sync: \
			return method();
