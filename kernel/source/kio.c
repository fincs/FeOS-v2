#include "common.h"

void kputs(const char* s)
{
	for (; *s; s++)
		kputc(*s);
}

void kputx(u32 x)
{
	static const char conv[] = "0123456789ABCDEF";

	int i;
	for (i = 32-4; i >= 0; i -= 4)
	{
		int v = (x >> i) & 0xF;
		kputc(conv[v]);
	}
}

static void _kputu(u32 x)
{
	u32 q = x / 10;
	u32 r = x % 10;
	if (q) _kputu(q);
	kputc('0' + r);
}

void kputu(u32 x)
{
	if (x == 0)
	{
		kputc('0');
		return;
	}

	_kputu(x);
}

void kputd(int x)
{
	if (x < 0)
	{
		kputc('-');
		x = -x;
	}
	_kputu(x);
}

void kprintf(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	kvprintf(fmt, va);
	va_end(va);
}

void kvprintf(const char* fmt, va_list va)
{
	const char* last;
	for (;;)
	{
		for (last = fmt; *fmt && *fmt != '%'; fmt++);
		KioWrite(last, fmt - last);
		if (!*fmt) break;
		int code = fmt[1]; fmt += 2;
		switch (code)
		{
			case 's':
				kputs(va_arg(va, const char*));
				break;
			case 'd':
				kputd(va_arg(va, int));
				break;
			case 'p':
				kputc('0'); kputc('x');
			case 'x':
				kputx(va_arg(va, u32));
				break;
			case 'u':
				kputu(va_arg(va, u32));
				break;
			case '%':
				kputc('%');
				break;
			default:
				kputs("INVALID");
				break;
		}
	}
}
