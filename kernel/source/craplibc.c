// Crap versions of C stdlib functions
// TODO: replace with optimized ASM versions

char* memcpy(char* dest, const char* src, unsigned int len)
{
	char* ret = dest;
	while (len--)
		*dest++ = *src++;
	return ret;
}

char* memset(char* ptr, int x, unsigned int len)
{
	char* ret = ptr;
	while (len--)
		*ptr++ = x;
	return ret;
}
