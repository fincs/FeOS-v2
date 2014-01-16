#include "common.h"

static inline const char* getDevRootName(KDevice* dev, const char* baseName)
{
	if (baseName)
		return baseName;

	switch (dev->GetType())
	{
		case DevType_Null:        return "null";
		case DevType_Stream:      return "strm";
		case DevType_Block:       return "blk";
		case DevType_Framebuffer: return "fb";
		case DevType_Network:     return "nic";
		default:                  return "dev";
	}
}

struct DevKind
{
	DevKind* next;
	DynArray arr;
	size_t firstFree, firstFreePos;
	semaphore_t mutex;
	char name[16];

	inline void* operator new (size_t sz) noexcept { return malloc(sz); }
	inline void operator delete (void* data) noexcept { free(data); }

	KDevice* getDevice(int i)
	{
		return (KDevice*)DynArray_Get(&arr, i);
	}

	int addDevice(KDevice* dev);
	void removeDevice(int i);

	DevKind(DevKind* n, const char* baseName) : next(n), firstFree(~0U), firstFreePos(0)
	{
		strncpy(name, baseName, sizeof(name));
		DynArray_Init(&arr);
		SemaphoreInit(&mutex, 1);
	}

	~DevKind()
	{
		for (size_t i = 0; i < firstFreePos; i ++)
		{
			auto dev = getDevice(i);
			if (dev) dev->Release();
		}
		DynArray_Free(&arr);
	}
};

static DevKind* firstKind;
static semaphore_t kindMutex = SEMAPHORE_STATIC_INIT(1);

static DevKind* GetDeviceKind(const char* baseName, bool create = false)
{
	if (!baseName)
		return nullptr;

	DevKind* k;
	for (k = firstKind; k; k = k->next)
	{
		if (strnicmp(k->name, baseName, sizeof(k->name)))
			continue;
		return k;
	}
	if (create)
	{
		SemaphoreDown(&kindMutex);
		k = new DevKind(firstKind, baseName);
		if (k)
			firstKind = k;
		SemaphoreUp(&kindMutex);
	}
	return k;
}

int DevKind::addDevice(KDevice* dev)
{
	int ret = -1;
	SemaphoreDown(&mutex);
	if (firstFree != ~0U)
	{
		// We have a free slot!
		ret = (int)firstFree;
		firstFree = (size_t)DynArray_Get(&arr, ret);
		DynArray_Set(&arr, ret, dev);
	} else if (DynArray_Set(&arr, firstFreePos, dev))
	{
		ret = firstFreePos ++;
		dev->AddRef();
	}
	SemaphoreUp(&mutex);
	return ret;
}

void DevKind::removeDevice(int i)
{
	SemaphoreDown(&mutex);
	auto dev = getDevice(i);
	if (dev)
	{
		dev->Release();
		DynArray_Set(&arr, i, (void*)firstFree);
		firstFree = i;
	}
	SemaphoreUp(&mutex);
}

struct tag_devCookie
{
	DevKind* kind;
	int id;
};

devCookie DevRegister(KDevice* dev, const char* baseName)
{
	if (!dev)
		return nullptr;

	devCookie cookie = (devCookie)malloc(sizeof(tag_devCookie));
	if (!cookie)
		return nullptr;

	cookie->kind = GetDeviceKind(getDevRootName(dev,baseName), true);
	if (!cookie->kind)
	{
		free(cookie);
		return nullptr;
	}

	cookie->id = cookie->kind->addDevice(dev);
	if (cookie->id < 0)
	{
		free(cookie);
		return nullptr;
	}

	return cookie;
}

void DevUnregister(devCookie cookie)
{
	if (!cookie)
		return;

	cookie->kind->removeDevice(cookie->id);
	free(cookie);
}

KDevice* DevGet(const char* baseName, int id)
{
	DevKind* k = GetDeviceKind(baseName);
	if (!k) return nullptr;

	KDevice* dev = k->getDevice(id);
	if (dev)
		dev->AddRef();
	return dev;
}
