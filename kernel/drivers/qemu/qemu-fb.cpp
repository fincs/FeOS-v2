#include "common.h"
#include <qemu/clcdc.h>

class QemuFbDrv : public KFramebufferImpl
{
	FbDevInfo m_info;
	void* m_buf;
	size_t m_bufSize;
	pageinfo_t* m_fbPage;
public:
	QemuFbDrv() : m_buf(nullptr), m_fbPage(nullptr)
	{
		m_info.structSize = sizeof(m_info);
	}

	~QemuFbDrv()
	{
		if (m_fbPage) MemFreePage(m_fbPage, PAGEORDER_1M);
	}

	bool Init();

	KDEVCMDMAP_DECLARE();

	intptr_t GetInfo(FbDevInfo& info)
	{
		memcpy(&info, &m_info, info.structSize);
		return 0;
	}

	intptr_t GetMem(void*& addr, size_t& size)
	{
		addr = m_buf;
		size = m_bufSize;
		return 0;
	}
};

KDEVCMDMAP_BEGIN(QemuFbDrv)
	KDEVCMDMAP_GetMem(GetMem)
	KFBCMDMAP_GetInfo(GetInfo)
KDEVCMDMAP_END()

bool QemuFbDrv::Init()
{
	// Allocate the framebuffer memory
	m_fbPage = MemAllocPage(PAGEORDER_1M);
	if (!m_fbPage)
	{
		kprintf("<QemuFbDrv> Could not allocate framebuffer memory!\n");
		return false;
	}

	// Hardcoded!
	m_info.width = 640;
	m_info.height = 480;
	m_info.stride = m_info.width*2;
	m_info.bitDepth = 16;
	m_buf = page2vphys(m_fbPage);
	m_bufSize = m_info.stride*m_info.height;

	SYS_OSCCLK4 = CLDC_VGA_CLK;
	CLCD_TIM0 = CLDC_VGA_TIM0;
	CLCD_TIM1 = CLDC_VGA_TIM1;
	CLCD_TIM2 = CLDC_VGA_TIM2;
	CLCD_FBADDR = virt2phys(m_buf);
	CLCD_CTRL = 0x1829;

	kprintf("<QemuFbDrv> Framebuffer at %p (w: %u; h: %u; stride: %u; size: %u)\n",
		m_buf, m_info.width, m_info.height, m_info.stride, m_bufSize);

	return true;
}

static devCookie regCookie;

static int ModuleAttach()
{
	QemuFbDrv* fb = new QemuFbDrv;
	if (fb)
	{
		if (fb->Init())
			regCookie = DevRegister(fb);
		fb->Release();
	}
	return fb ? 0 : -ENOMEM;
}

static void ModuleDetach()
{
	if (regCookie)
		DevUnregister(regCookie);
}

FEOS_EXPORTMODULE(QemuFramebuffer)
{
	.Attach = ModuleAttach,
	.CanUnload = DefModCanUnload,
	.Detach = ModuleDetach,
};
