#pragma once
#ifndef FEOS_DMODEL_INCLUDED
#error "You must include <dmodel.h> first!"
#endif

template <int devType, word_t& usageCount>
class KDeviceImpl : public KDevice
{
protected:
	word_t m_refCount;
public:
	KDeviceImpl() : m_refCount(1) { }
	virtual ~KDeviceImpl() { }

	// Non-throwing object creation and destruction
	inline void* operator new (size_t sz) noexcept { return malloc(sz); }
	inline void operator delete (void* data) noexcept { free(data); }

	int GetType()
	{
		return devType;
	}

	word_t AddRef()
	{
		AtomicIncrement(&usageCount);
		return AtomicIncrement(&m_refCount);
	}

	word_t Release()
	{
		AtomicDecrement(&usageCount);
		word_t ret = AtomicDecrement(&m_refCount);
		if (!ret)
			delete this;
		return ret;
	}

	intptr_t Command(int cmdId, intptr_t arg1, intptr_t arg2)
	{
		return -ENOTSUP;
	}
};

template <typename T>
static inline intptr_t DevGetMem(KDevice* dev, T*& mem, size_t& memSize)
{
	return dev->Command(DevCmd_GetMem, (intptr_t)&mem, (intptr_t)&memSize);
}

#define KDEVCMDMAP_DECLARE() \
	intptr_t Command(int cmdId, intptr_t arg1, intptr_t arg2)

#define KDEVCMDMAP_BEGIN(className) \
	intptr_t className::Command(int cmdId, intptr_t arg1, intptr_t arg2) \
	{ \
		switch (cmdId) \
		{

#define KDEVCMDMAP_GetMem(method) \
			case DevCmd_GetMem: \
				return method(*((void**)arg1), *((size_t*)arg2));

#define KDEVCMDMAP_END() \
		} \
		return -ENOTSUP; \
	}
