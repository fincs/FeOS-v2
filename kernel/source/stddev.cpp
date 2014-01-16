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

extern "C" void DevInit()
{
	KIODriver* kio = new KIODriver;
	if (kio)
	{
		DevRegister(kio);
		kio->Release();
	}
}

extern "C" void DevTest()
{
	KStream* strm = KStream::From(DevGet("strm", 0));
	if (!strm)
	{
		kputs("<DevTest> Can't get strm0 device!\n");
		return;
	}

	strm->Write("Works!", 6);
	strm->Release();
}
