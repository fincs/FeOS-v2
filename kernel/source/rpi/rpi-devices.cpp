#include "../common.h"
#include <rpi/vcprops.h>

static void initVCMem()
{
	VCPROPS props;
	if (!vcPropsAlloc(&props, 256))
	{
		kputs("Cannot allocate VCProps buffer!\n");
		return;
	}

	vu32* tag = vcPropsAddTag(&props, VCTag_GetVCMem, 0, 2*4);

	if (!vcPropsExecute(&props))
	{
		vcPropsFree(&props);
		kputs("Mailbox request error!\n");
		return;
	}

	u32 vcMemStart = tag[0];
	u32 vcMemSize = tag[1];
	vcPropsFree(&props);

	int startIdx = L1_INDEX(phys2virt(vcMemStart));
	int endIdx = startIdx + (vcMemSize>>20);
	u32 curPos = vcMemStart - BIT(20);
	int i;
	for (i = startIdx; i < endIdx; i ++)
		MASTER_PAGETABLE[i] = MMU_L1_SECT1MB | (curPos+=BIT(20)) | MMU_L1_RWNA | MMU_L1_UNCACHED;

	kprintf("VC memory @ addr %p (%u bytes)\n", vcMemStart, vcMemSize);
}

void DevInitPlatform()
{
	initVCMem();

	LINK_MODULE(RPiFramebuffer);
}
