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
	LINK_MODULE(InitRdDrv);

	IoInit();
}
