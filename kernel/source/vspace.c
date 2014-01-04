#include "common.h"

typedef struct tag_vspacetoken vspacetoken_t;

struct tag_vspacetoken
{
	vspacetoken_t* next;
	vspacetoken_t* prev;
	u32 flags, pcount;
};

enum
{
	VSPACEF_USED = BIT(0),
	VSPACEF_FMASK = 0xFFF,
};

HVSPACE vspace_create(void* addr, u32 size)
{
	vspacetoken_t* t = (vspacetoken_t*)malloc(sizeof(vspacetoken_t));
	if (!t) return nullptr;

	t->next = nullptr;
	t->prev = nullptr;
	t->flags = (u32)addr &~ VSPACEF_FMASK;
	t->pcount = size >> 12;
	return (HVSPACE)t;
}

void vspace_freeAll(HVSPACE h)
{
	vspacetoken_t* t = (vspacetoken_t*)h;
	if (t->prev) return; // Invalid operation
	while (t)
	{
		vspacetoken_t* next = t->next;
		free(t);
		t = next;
	}
}

HVSPACE vspace_alloc(HVSPACE h, u32 size)
{
	size = (size + 0xFFF) >> 12;
#ifdef DEBUG
	kprintf("<vspace_alloc> Caller requested %u pages\n", size);
#endif

	vspacetoken_t* t;
	for (t = (vspacetoken_t*)h; t; t = t->next)
	{
		if (t->flags & VSPACEF_USED) continue;
		if (t->pcount < size) continue;

		if (t->pcount == size)
		{
			// Perfect match
			t->flags |= VSPACEF_USED;
			break;
		}

		// Else we need to divide this block into two pieces - the first free and the second used.
		vspacetoken_t* t2 = (vspacetoken_t*)malloc(sizeof(vspacetoken_t));
		if (!t2) return nullptr;
		u32 rems = t->pcount - size;
		t2->prev = t;
		t2->next = t->next;
		if (t->next)
			t->next->prev = t2;
		t->next = t2;
		t->pcount = rems;
		t2->flags = (t->flags + (rems << 12)) | VSPACEF_USED;
		t2->pcount = size;
		t = t2;
		break;
	}

	return (HVSPACE)t;
}

void vspace_free(HVSPACE vs)
{
	vspacetoken_t* t = (vspacetoken_t*)vs;
	if (!(t->flags & VSPACEF_USED)) return; // Invalid operation
	t->flags = t->flags &~ VSPACEF_USED;
	vspacetoken_t* t2;

	// Coalesce on the RIGHT
	if ((t2 = t->next) && !(t2->flags & VSPACEF_USED)) // need to do it only once
	{
#ifdef DEBUG
		kputs("<vspace_free> Coalescing on the right...\n");
#endif
		t->pcount += t2->pcount;
		t->next = t2->next;
		if (t2->next) t2->next->prev = t;
		free(t2);
	}

	// Coalesce on the LEFT
	if ((t2 = t->prev) && !(t2->flags & VSPACEF_USED)) // need to do it only once
	{
#ifdef DEBUG
		kputs("<vspace_free> Coalescing on the left...\n");
#endif
		t2->pcount += t->pcount;
		t2->next = t->next;
		if (t->next) t->next->prev = t2;
		free(t);
	}
}

u32 vspace_getAddr(HVSPACE vs)
{
	return ((vspacetoken_t*)vs)->flags &~ VSPACEF_FMASK;
}

u32 vspace_getPageCount(HVSPACE vs)
{
	return ((vspacetoken_t*)vs)->pcount;
}
