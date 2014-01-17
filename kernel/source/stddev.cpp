#include "common.h"

bool DefModCanUnload()
{
	return g_defModUsageRefCnt == 0;
}

class KIODriver : public KStreamImpl
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

	LINK_MODULE(ConsoleDrv);
}

static inline intptr_t KStrmPrint(KStream* strm, const char* buf)
{
	return strm->Write(buf, strlen(buf));
}

void DevTest()
{
	KFramebuffer* fb = KFramebuffer::From(DevGet("fb"));
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

	KStream* con = KStream::From(DevGet("con"));
	if (con)
	{
		kputs("Got console device\n");
		KStrmPrint(con, "FeOS/RPi Console Demo\n");
		KStrmPrint(con, "Hello World!\n");
		KStrmPrint(con, "Te\tst\tting tabs\n\n");
		KStrmPrint(con, "\tHello World #FeOS\n");
		KStrmPrint(con, "\thttp://feos.mtheall.com\n");
		con->Release();
	}
}
