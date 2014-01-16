#include "common.h"

bool DynArray_Set(DynArray* a, size_t pos, void* data)
{
	if (pos < a->size)
		a->data[pos] = data;
	else
	{
		size_t dbl = a->size * 2;
		size_t newSize = dbl > pos ? dbl : (pos+1);
		void** temp = (void**) realloc(a->data, sizeof(void*)*newSize);
		if (!temp)
			return false;
		else
		{
			a->data = temp;
			a->data[pos] = data;
		}
	}
	return true;
}
