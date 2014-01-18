#include "common.h"

static int _fputx(u32 x, FILE* f)
{
	static const char conv[] = "0123456789ABCDEF";

	int i;
	int ret = 0;
	for (i = 32-4; i >= 0; i -= 4)
	{
		int v = (x >> i) & 0xF;
		int ret2 = fputc(conv[v], f);
		if (ret2 < 0) return EOF;
		ret += ret2;
	}
	return ret;
}

static int __fputu(u32 x, FILE* f)
{
	u32 q = x / 10;
	u32 r = x % 10;
	int ret = 0;
	if (q)
		ret = __fputu(q, f);
	if (ret < 0) return ret;
	int ret2 = fputc('0' + r, f);
	return ret2 < 0 ? EOF : (ret+ret2);
}

static int _fputu(u32 x, FILE* f)
{
	if (x == 0)
		return fputc('0', f);

	return __fputu(x, f);
}

static int _fputd(int x, FILE* f)
{
	int ret = 0;
	if (x < 0)
	{
		ret = fputc('-', f);
		if (ret < 0) return EOF;
		x = -x;
	}
	int ret2 = _fputu(x, f);
	return ret2 < 0 ? EOF : (ret+ret2);
}

int vfprintf(FILE* f, const char* fmt, va_list va)
{
	if (!f)
		return EOF;

	int ret = 0;
	const char* last;
	while (ret >= 0)
	{
		for (last = fmt; *fmt && *fmt != '%'; fmt++);
		size_t size = fwrite(last, 1, fmt - last, f);
		if (!size) return -1;
		ret += (int)size;
		if (!*fmt) break;
		int code = fmt[1]; fmt += 2;
#define HANDLE(x) do { int d = (x); ret = (d>=0) ? (ret+d) : -1; } while(0)
		switch (code)
		{
			case 's':
				HANDLE(fputs(va_arg(va, const char*),f));
				break;
			case 'd':
				HANDLE(_fputd(va_arg(va, int),f));
				break;
			case 'p':
				HANDLE(fputs("0x",f));
				if (ret < 0) break;
			case 'x':
				HANDLE(_fputx(va_arg(va, u32),f));
				break;
			case 'u':
				HANDLE(_fputu(va_arg(va, u32),f));
				break;
			case 'c':
				HANDLE(fputc(va_arg(va, int),f));
				break;
			case '%':
				HANDLE(fputc('%',f));
				break;
			default:
				kputs("INVALID");
				break;
		}
	}
	return ret;
}

int fprintf(FILE* f, const char* fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	int rc = vfprintf(f, fmt, v);
	va_end(v);
	return rc;
}

int printf(const char* fmt, ...)
{
	va_list v;
	va_start(v, fmt);
	int rc = vfprintf(stdout, fmt, v);
	va_end(v);
	return rc;
}

int vprintf(const char* fmt, va_list v)
{
	return vfprintf(stdout, fmt, v);
}
