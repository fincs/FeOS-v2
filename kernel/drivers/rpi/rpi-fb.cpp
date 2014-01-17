#include "common.h"
#include <rpi/vcprops.h>

word_t g_rpiFbCnt;

class RPiFbDrv : public KFramebufferImpl<g_rpiFbCnt>
{
	FbDevInfo m_info;
	void* m_buf;
	size_t m_bufSize;
public:
	RPiFbDrv() : m_buf(nullptr)
	{
		m_info.structSize = sizeof(m_info);
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

KDEVCMDMAP_BEGIN(RPiFbDrv)
	KDEVCMDMAP_GetMem(GetMem)
	KFBCMDMAP_GetInfo(GetInfo)
KDEVCMDMAP_END()

bool RPiFbDrv::Init()
{
	bool rc = false;

	VCPROPS props;
	if (!vcPropsAlloc(&props, 256))
	{
		kputs("<RPiFbDrv> Cannot allocate VCProps buffer\n");
		return rc;
	}

	vu32* tag = vcPropsAddTag(&props, VCTag_FB_GetPhysSize, 0, 2*4);
	if (!vcPropsExecute(&props))
	{
		kputs("<RPiFbDrv> Cannot retrieve physical size!\n");
		goto _return;
	}

	m_info.width = tag[0];
	m_info.height = tag[1];
	m_info.bitDepth = 16; // hardcoded!
	if (!m_info.width || !m_info.height)
		m_info.width = 640, m_info.height = 480;

	vcPropsReset(&props);
	/* Redundant?
	tag = vcPropsAddTag(&props, VCTag_FB_SetPhysSize, 2*4, 2*4);
	tag[0] = m_info.width;
	tag[1] = m_info.height;
	*/
	tag = vcPropsAddTag(&props, VCTag_FB_SetVirtSize, 2*4, 2*4);
	tag[0] = m_info.width;
	tag[1] = m_info.height;
	tag = vcPropsAddTag(&props, VCTag_FB_SetBPP, 4, 4);
	tag[0] = m_info.bitDepth;
	tag = vcPropsAddTag(&props, VCTag_AllocFB, 4, 2*4);
	tag[0] = 4096; // we want a page-aligned framebuffer

	if (!vcPropsExecute(&props))
	{
		kputs("<RPiFbDrv> Cannot set up framebuffer!\n");
		goto _return;
	}

	m_buf = phys2virt(tag[0]);
	m_bufSize = tag[1];

	vcPropsReset(&props);
	tag = vcPropsAddTag(&props, VCTag_FB_GetStride, 0, 4);
	if (!vcPropsExecute(&props))
	{
		kputs("<RPiFbDrv> Cannot get the stride of the framebuffer!\n");
		goto _return;
	}

	m_info.stride = tag[0];
	rc = true;

	kprintf("<RPiFbDrv> Framebuffer at %p (w: %u; h: %u; stride: %u; size: %u)\n",
		m_buf, m_info.width, m_info.height, m_info.stride, m_bufSize);

_return:
	vcPropsFree(&props);
	return rc;
}

static devCookie regCookie;

static int ModuleAttach()
{
	RPiFbDrv* fb = new RPiFbDrv;
	if (fb)
	{
		if (fb->Init())
			regCookie = DevRegister(fb);
		fb->Release();
	}
	return fb ? 0 : -ENOMEM;
}

static bool ModuleCanUnload()
{
	return g_rpiFbCnt == 0;
}

static void ModuleDetach()
{
	if (regCookie)
		DevUnregister(regCookie);
}

FEOS_EXPORTMODULE(RPiFramebuffer)
{
	.Attach = ModuleAttach,
	.CanUnload = ModuleCanUnload,
	.Detach = ModuleDetach,
};
