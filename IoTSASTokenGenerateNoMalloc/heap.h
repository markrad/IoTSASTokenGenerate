#pragma once

#define _DEBUG_HEAP

#include <stdint.h>

typedef void* HEAPHANDLE;

HEAPHANDLE heapInit(uint8_t *buffer, size_t bufferLen);
void* heapMalloc(HEAPHANDLE hHeap, size_t bytes);
void heapFree(HEAPHANDLE hHeap, void* address);
void* heapRealloc(HEAPHANDLE hHeap, void* address, uint16_t newLength);

#ifdef _DEBUG_HEAP
void heapSanity(HEAPHANDLE hHeap);
#endif

