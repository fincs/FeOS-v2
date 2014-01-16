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

int strlen(const char* buf)
{
	int i = 0;
	for (; *buf++; i ++);
	return i;
}

int strnicmp(const char* a, const char* b, unsigned int maxlen)
{
	unsigned int pos;
	for (pos = 0; pos < maxlen; pos ++)
	{
		int ca = *a++, cb = *b++;
		if (!ca && !cb)
			break;
		if (ca >= 'a' && ca <= 'z') ca += 'A' - 'a';
		if (cb >= 'a' && cb <= 'z') cb += 'A' - 'a';
		if (ca != cb)
			return ca-cb;
	}
	return 0;
}

char* strncpy(char* dst, const char* src, unsigned int maxlen)
{
	char* ret = dst;
	unsigned int pos;
	for (pos = 0; pos < maxlen; pos ++)
	{
		int c = *src++;
		*dst++ = c;
		if (!c)
			break;
	}
	return ret;
}
