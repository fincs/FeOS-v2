#include "common.h"

static FILE* stdStrm[3];

FILE* IoGetStdin() { return stdStrm[0]; }
FILE* IoGetStdout() { return stdStrm[1]; }
FILE* IoGetStderr() { return stdStrm[2]; }

struct tag_FILE
{
	KStream* strm;
	int mode;
	int error;
	u8* buf;
	int bufMode;
	bool bAtEOF;

	// Non-throwing object creation and destruction
	inline void* operator new (size_t sz) noexcept { return malloc(sz); }
	inline void operator delete (void* data) noexcept { free(data); }

	tag_FILE(KStream* strm) : strm(strm), mode(0), error(0), buf(nullptr), bufMode(_IONBF), bAtEOF(false) { strm->AddRef(); }
};

FILE* fopendev(const char* devName, int id)
{
	auto dev = KStream::From(DevGet(devName, id));
	if (!dev) return nullptr;

	FILE* f = new tag_FILE(dev);
	dev->Release();
	return f;
}

void IoInit()
{
	stdStrm[1] = fopendev("con", -1);
}

int fflush(FILE* f)
{
	if (!f) return EOF;

	return 0;
}

int fclose(FILE* f)
{
	if (!f) return EOF;

	int ret = fflush(f);
	f->strm->Release();
	if (f->buf)
		free(f->buf);
	delete f;
	return ret;
}

size_t fread(void* buf, size_t elemSize, size_t elemCount, FILE* f)
{
	if (!f) return 0;

	size_t size = elemSize*elemCount;

	auto ret = f->strm->Read(buf, size);
	f->error = ret < 0 ? (-ret) : 0;
	return ret >= 0 ? ret : 0;
}

size_t fwrite(const void* buf, size_t elemSize, size_t elemCount, FILE* f)
{
	if (!f) return 0;

	size_t size = elemSize*elemCount;
	auto ret = f->strm->Write(buf, size);
	f->error = ret < 0 ? (-ret) : 0;
	return ret >= 0 ? ret : 0;
}

int fseek(FILE* f, int pos32, int mode)
{
	if (!f) return -1;

	u64 pos = pos32;

	f->error = -f->strm->Seek(pos, mode);
	return f->error > 0 ? -1 : 0;
}

int ftell(FILE* f)
{
	if (!f) return -1;

	u64 pos = 0;

	f->error = -f->strm->Seek(pos, StrmDevSeek_Cur);
	return f->error > 0 ? -1 : (int)pos;
}

int ferror(FILE* f)
{
	if (!f) return 0;

	return f->error;
}

void clearerr(FILE* f)
{
	if (f)
		f->error = 0;
}

int fputs(const char* buf, FILE* f)
{
	if (!f) return EOF;

	size_t bufSize = strlen(buf);
	size_t size = fwrite(buf, 1, bufSize, f);
	return size == bufSize ? (int)size : EOF;
}

int fputc(int c, FILE* f)
{
	if (!f) return EOF;

	u8 b = (u8)c;
	size_t size = fwrite(&b, 1, 1, f);
	return size == 1 ? 1 : EOF;
}

int puts(const char* str)
{
	int ret = fputs(str, stdout);
	if (ret < 0)
		return ret;
	int ret2 = fputc('\n', stdout);
	if (ret2 < 0)
		return ret2;
	return ret + ret2;
}
