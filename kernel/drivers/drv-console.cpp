#include "common.h"
#include "consolefont.h"

class ConsoleDrv : public KStreamImpl
{
	StrmDevStat m_stat;
	KFramebuffer* m_fb;
	FbDevInfo m_fbInfo;

	u8* m_conFb;
	int m_conW, m_conH;
	int m_conX, m_conY;

	void newline();
	void drawChar(int x, int y, int ch);

public:
	ConsoleDrv() : m_conX(0), m_conY(0)
	{
		m_stat.structSize = sizeof(m_stat);
		m_stat.flags = StrmDev_Writable;
		m_stat.size = 0; // don't care
		m_fbInfo.structSize = sizeof(m_fbInfo);
	}

	~ConsoleDrv()
	{
		if (m_fb) m_fb->Release();
	}

	bool Init();

	KDEVCMDMAP_DECLARE();

	intptr_t GetStat(StrmDevStat& stat)
	{
		memcpy(&stat, &m_stat, stat.structSize);
		return 0;
	}

	intptr_t Write(const void* buffer, size_t bufSize);
};

KDEVCMDMAP_BEGIN(ConsoleDrv)
	KSTRMCMDMAP_GetStat(GetStat)
	KSTRMCMDMAP_Write(Write)
KDEVCMDMAP_END()

bool ConsoleDrv::Init()
{
	m_fb = KFramebuffer::From(DevGet("fb"));
	if (!m_fb)
	{
		kputs("<ConsoleDrv> Cannot open framebuffer!");
		return false;
	}
	if (m_fb->GetInfo(m_fbInfo) < 0)
	{
		kputs("<ConsoleDrv> Cannot retrieve framebuffer properties!");
		return false;
	}

	size_t dummy;
	if (DevGetMem(m_fb, m_conFb, dummy) < 0)
	{
		kputs("<ConsoleDrv> Cannot obtain framebuffer memory!");
		return false;
	}

	m_conW = m_fbInfo.width / FONT_CHARW;
	m_conH = m_fbInfo.height / FONT_CHARH;
	kprintf("<ConsoleDrv> Setting up %dx%d console...\n", m_conW, m_conH);

	return true;
}

intptr_t ConsoleDrv::Write(const void* buffer, size_t bufSize)
{
	const char* buf = (const char*)buffer;
	const char* bufEnd = buf + bufSize;
	while (buf != bufEnd)
	{
		int c = *buf++;
		if (c == '\n')
			newline();
		else if (c == '\t')
		{
			m_conX = (m_conX&~7) + 8;
			if (m_conX >= m_conW)
				newline();
		} else
		{
			drawChar(m_conX++, m_conY, c);
			if (m_conX >= m_conW)
				newline();
		}
	}
	return bufSize;
}

void ConsoleDrv::newline()
{
	m_conX = 0;
	m_conY ++;
	if (m_conY >= m_conH)
	{
		m_conY --;
		int stride = m_fbInfo.stride;
		memcpy(m_conFb, m_conFb + FONT_CHARH*stride, (m_conH-1)*stride);
		memset(m_conFb + m_conY*stride, 0, stride);
	}
}

void ConsoleDrv::drawChar(int x, int y, int ch)
{
	int stride = m_fbInfo.stride;
	// TODO: do not hardcode bitdepth
	u16* buf = (u16*)(m_conFb + y*FONT_CHARH*stride) + x*FONT_CHARW;
	const u8* fnt = fontData + ch*((FONT_CHARW+7)/8)*FONT_CHARH;
	for (int j = 0; j < FONT_CHARH; j ++)
	{
		for (int i = 0; i < FONT_CHARW; i ++)
		{
			int x = fnt[i/8] & BIT(8-(i&7));
			if (x)
				buf[i] = 0xFFFF;
			else
				buf[i] = 0;
		}
		fnt += ((FONT_CHARW+7)/8);
		buf += stride/2;
	}
}

static devCookie regCookie;

static int ModuleAttach()
{
	ConsoleDrv* con = new ConsoleDrv;
	if (con)
	{
		if (con->Init())
			regCookie = DevRegister(con, "con");
		con->Release();
	}
	return con ? 0 : -ENOMEM;
}

static void ModuleDetach()
{
	if (regCookie)
		DevUnregister(regCookie);
}

FEOS_EXPORTMODULE(ConsoleDrv)
{
	.Attach = ModuleAttach,
	.CanUnload = DefModCanUnload,
	.Detach = ModuleDetach,
};
