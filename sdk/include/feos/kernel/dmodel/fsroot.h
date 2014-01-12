#pragma once
#pragma interface
#ifndef FEOS_DMODEL_INCLUDED
#error "You must include <dmodel.h> first!"
#endif

#define FSROOT_INTERFACE_VER 1

struct FSRootInfo
{
	size_t cwdInfoSize;
	size_t dirIterSize;
	word_t ifaceVer;
	word_t flags;
};

struct OpenParams
{
	KDevice* pDevice; // output parameter
	void* cwdInfo; // CWD info - may be null to specify default CWD
};

struct DirEntry
{
	size_t structSize;
	size_t nameBufSize;
	// TODO: fields
};

struct FSStats
{
	size_t structSize;
	// TODO: fields
};

struct IFSRoot
{
	virtual FSRootInfo* GetInfo() = 0;
	virtual word_t AddRef() = 0;
	virtual word_t Release() = 0;

	virtual int Open(OpenParams& params, const char* path, int mode) = 0;
	virtual int GetStats(FSStats& stats) = 0;

	virtual int Move(void* /*optional*/ cwdInfo, const char* path, const char* newPath) = 0;
	virtual int Link(void* /*optional*/ cwdInfo, const char* path, const char* newPath) = 0;
	virtual int Unlink(void* /*optional*/ cwdInfo, const char* path) = 0;

	virtual int CwdNew(void* cwdInfo, void* /*optional*/ cwdBase) = 0;
	virtual int CwdSet(void* cwdInfo, const char* path) = 0;
	virtual int CwdGet(void* cwdInfo, char* buffer, size_t bufSize) = 0;
	virtual int CwdFree(void* cwdInfo);

	virtual int DirOpen(void* /*optional*/ cwdInfo, void* dirIter, const char* path) = 0;
	virtual int DirReset(void* dirIter) = 0;
	virtual int DirNext(void* dirIter, DirEntry& entry, char* buffer) = 0;
	virtual int DirClose(void* dirIter) = 0;
};
