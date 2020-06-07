#pragma once

//#define _DEBUG_HEAP

#include <stdint.h>
#include <stddef.h>

typedef void* HEAPHANDLE;

typedef struct _HEAPINFO
{
	int freeBytes;
	int usedBytes;
	int totalBytes;
	int largestFree;
} HEAPINFO;

HEAPHANDLE heapInit(uint8_t *buffer, size_t bufferLen);
void* heapMalloc(HEAPHANDLE hHeap, size_t bytes);
void heapFree(HEAPHANDLE hHeap, void* address);
void* heapRealloc(HEAPHANDLE hHeap, void* address, uint16_t newLength);
void heapGetInfo(HEAPHANDLE hHeap, HEAPINFO *heapInfo);

#ifdef _DEBUG_HEAP
void heapSanity(HEAPHANDLE hHeap);
#endif

