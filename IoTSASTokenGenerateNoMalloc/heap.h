#pragma once

#include <stdint.h>

typedef struct _MEMORYBLOCK
{
	uint16_t length;
	uint16_t next;
	uint16_t previous;
} MEMORYBLOCKSTRUCT, * MEMORYBLOCK;

typedef struct _HEAPHANDLE
{
	MEMORYBLOCKSTRUCT freeList;
	MEMORYBLOCKSTRUCT usedList;
} HEAPHANDLESTRUCT, *HEAPHANDLE;

HEAPHANDLE heapInit(uint8_t *buffer, size_t bufferLen);
void* heapMalloc(HEAPHANDLE hHeap, size_t bytes);
void heapFree(HEAPHANDLE hHeap, void* address);

#ifdef _DEBUG
void heapSanity(HEAPHANDLE hHeap);
#endif

