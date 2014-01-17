#include "common.h"

word_t g_stdDevRefCnt;

class KIODriver : public KStreamImpl<g_stdDevRefCnt>
{
	StrmDevStat m_stat;
public:
	KIODriver()
	{
		m_stat.structSize = sizeof(m_stat);
		m_stat.flags = StrmDev_Writable;
		m_stat.size = 0; // don't care
	}

	KDEVCMDMAP_DECLARE();

	intptr_t GetStat(StrmDevStat& stat)
	{
		memcpy(&stat, &m_stat, stat.structSize);
		return 0;
	}

	intptr_t Write(const void* buffer, size_t bufSize)
	{
		KioWrite(buffer, bufSize);
		return bufSize;
	}
};

KDEVCMDMAP_BEGIN(KIODriver)
	KSTRMCMDMAP_GetStat(GetStat)
	KSTRMCMDMAP_Write(Write)
KDEVCMDMAP_END()

void DevInit()
{
	DevInitPlatform();

	KIODriver* kio = new KIODriver;
	if (kio)
	{
		DevRegister(kio);
		kio->Release();
	}
}

void DevTest()
{
	KStream* strm = KStream::From(DevGet("strm", 0));
	if (!strm)
	{
		kputs("<DevTest> Can't get strm0 device!\n");
		return;
	}

	strm->Write("Works!", 6);
	strm->Release();

	KFramebuffer* fb = KFramebuffer::From(DevGet("fb", 0));
	if (fb)
	{
		FbDevInfo info = { sizeof(FbDevInfo) };
		if (fb->GetInfo(info) >= 0 && info.bitDepth == 16)
		{
			u16* buf;
			size_t bufSize;
			if (DevGetMem(fb, buf, bufSize) >= 0)
			{
				kprintf("Got FB: %p\n", buf);
				int i, j;
				u16* line = buf;
				for (j = 0; j < (int)info.height; j ++)
				{
					for (i = 0; i < (int)info.width; i ++)
					{
						line[i] = (i*j)&0xFFFF;
					}
					line = (u16*)((char*)line + info.stride);
				}
			}
		}
		fb->Release();
	}
}
