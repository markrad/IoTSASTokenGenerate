#include <stdbool.h>

#ifdef _DEBUG_HEAP
#include <stdio.h>
#include <memory.h>
#endif

#include "heap.h"

#define MIN_ALLOC sizeof(MEMORYBLOCKSTRUCT) + 4
#define CHAIN_END UINT16_MAX
#define MIN_BUFFER 1024
#define MAX_BUFFER UINT16_MAX

MEMORYBLOCK heapGetFreeList(HEAPHANDLE hHeap);
MEMORYBLOCK heapGetUsedList(HEAPHANDLE hHeap);
uint16_t heapGetOffset(HEAPHANDLE hHeap, MEMORYBLOCK mb);
MEMORYBLOCK heapGetNextAddress(HEAPHANDLE hHead, MEMORYBLOCK mb);
MEMORYBLOCK heapGetPreviousAddress(HEAPHANDLE hHead, MEMORYBLOCK mb);
MEMORYBLOCK heapGetMB(uint8_t* address);
uint8_t* heapGetData(MEMORYBLOCK mb);
void heapInsertAfter(HEAPHANDLE hHeap, MEMORYBLOCK target, MEMORYBLOCK newItem);
void heapRemoveFromList(HEAPHANDLE hHeap, MEMORYBLOCK mb);
int heapGetIsAdjacent(MEMORYBLOCK first, MEMORYBLOCK second);

#ifdef _DEBUG_HEAP
void heapSanity(HEAPHANDLE hHeap);
#endif

// Initialize the heap structures
HEAPHANDLE heapInit(uint8_t *buffer, size_t bufferLen)
{
	if (buffer == NULL || bufferLen < MIN_BUFFER || bufferLen > MAX_BUFFER)
		return NULL;

#ifdef _DEBUG_HEAP
	memset(buffer, 0xee, bufferLen);
#endif

	HEAPHANDLE hHeap = (HEAPHANDLE)buffer;

	hHeap->usedList.length = 0;
	hHeap->usedList.next = CHAIN_END;
	hHeap->usedList.previous = CHAIN_END;
	hHeap->freeList.length = 1;
	hHeap->freeList.next = (uint16_t)sizeof(HEAPHANDLESTRUCT);
	hHeap->freeList.previous = CHAIN_END;

	MEMORYBLOCK first = heapGetNextAddress(hHeap, &hHeap->freeList);

	first->length = (uint16_t)(bufferLen - sizeof(HEAPHANDLESTRUCT) - sizeof(MEMORYBLOCKSTRUCT));
	first->next = CHAIN_END;
	first->previous = (uint16_t)((uint8_t*)&hHeap->freeList - buffer);

#ifdef _DEBUG_HEAP
	heapSanity(hHeap);
#endif

	return hHeap;
}

// Allocate a block
void* heapMalloc(HEAPHANDLE hHeap, size_t bytes)
{
	if (hHeap == NULL || bytes == 0)
		return NULL;

	if (bytes % 2 != 0)
		bytes += 1;

	MEMORYBLOCK mb = heapGetFreeList(hHeap);

	while (NULL != (mb = heapGetNextAddress(hHeap, mb)))
	{
		if (mb->length > bytes)
			break;
	}

	if (mb != NULL)
	{
		if (mb->length - bytes > MIN_ALLOC)
		{
			MEMORYBLOCK add = (MEMORYBLOCK)(heapGetData(mb) + bytes);
			add->next = mb->next;
			add->previous = mb->previous;
			heapGetPreviousAddress(hHeap, mb)->next = heapGetOffset(hHeap, add);

			if (add->next != CHAIN_END)
				heapGetNextAddress(hHeap, add)->previous = heapGetOffset(hHeap, add);

			add->length = mb->length - sizeof(MEMORYBLOCKSTRUCT) - (uint16_t)bytes;
			mb->length = (uint16_t)bytes;
		}
		else
		{
			heapRemoveFromList(hHeap, mb);
		}

		heapInsertAfter(hHeap, heapGetUsedList(hHeap), mb);

#ifdef _DEBUG_HEAP
		heapSanity(hHeap);
#endif
		return heapGetData(mb);
	}
	else
	{
#ifdef _DEBUG_HEAP
		heapSanity(hHeap);
#endif
		return NULL;
	}
}

// Free an allocated block
void heapFree(HEAPHANDLE hHeap, void* address)
{
	if (hHeap == NULL || address == NULL)
		return;

	MEMORYBLOCK mb = heapGetNextAddress(hHeap, heapGetUsedList(hHeap));

	while (mb != NULL)
	{
		if (heapGetData(mb) == (uint8_t *)address)
			break;

		mb = heapGetNextAddress(hHeap, mb);
	}

	if (mb == NULL)
		return;

	heapRemoveFromList(hHeap, mb);

#ifdef _DEBUG_HEAP
	memset(heapGetData(mb), 0xee, mb->length);

	printf("check\r\n");
	heapSanity(hHeap);
	heapRemoveFromList(hHeap, mb);
	heapSanity(hHeap);
#endif

	MEMORYBLOCK search = heapGetNextAddress(hHeap, heapGetFreeList(hHeap));

	while (search != NULL)
	{
		int adj = heapGetIsAdjacent(search, mb);

		switch (adj)
		{
		case 0:
			break;
		case -1:
			// Is before freed entry - add search to current
			search->length += (mb->length + sizeof(MEMORYBLOCKSTRUCT));
			mb = search;
			heapRemoveFromList(hHeap, mb);
			search = heapGetFreeList(hHeap);
			break;
		case 1:
			// Is after freed entry - add current to search
			heapRemoveFromList(hHeap, search);
			mb->length += (search->length + sizeof(MEMORYBLOCKSTRUCT));
			search = heapGetFreeList(hHeap);
			break;
		default:
			break;
		}

		search = heapGetNextAddress(hHeap, search);
	}

	MEMORYBLOCK target = NULL;

	for (target = heapGetFreeList(hHeap); 
		NULL != heapGetNextAddress(hHeap, target) && heapGetNextAddress(hHeap, target)->length < mb->length; 
		target = heapGetNextAddress(hHeap, target))
	{	}

	heapInsertAfter(hHeap, target, mb);


#ifdef _DEBUG_HEAP
	heapSanity(hHeap);
#endif
}

#ifdef _DEBUG_HEAP
void heapSanity(HEAPHANDLE hHeap)
{
	MEMORYBLOCK mb;

	int freeBytes = 0;
	int usedBytes = 0;
	int totalBytes = 0;
	int largestFree = 0;

	printf("\r\nUsed list\r\n\n");

	mb = heapGetUsedList(hHeap);
	
	while (NULL != (mb = heapGetNextAddress(hHeap, mb)))
	{
		printf("offset=%d;next=%d;previous=%d;length=%d\r\n", heapGetOffset(hHeap, mb), mb->next, mb->previous, mb->length);
		totalBytes += mb->length + sizeof(MEMORYBLOCKSTRUCT);
		usedBytes += mb->length;
	}

	printf("\r\nFree list\r\n\n");

	mb = heapGetFreeList(hHeap);

	while (NULL != (mb = heapGetNextAddress(hHeap, mb)))
	{
		printf("offset=%d;next=%d;previous=%d;length=%d\r\n", heapGetOffset(hHeap, mb), mb->next, mb->previous, mb->length);
		totalBytes += mb->length + sizeof(MEMORYBLOCKSTRUCT);
		freeBytes += mb->length;

		if (mb->length > largestFree)
			largestFree = mb->length;
	}

	printf("\nbytes accounted for = %05d\n", (int)(totalBytes + sizeof(HEAPHANDLESTRUCT)));
	printf("         free bytes = %05d\n", freeBytes);
	printf("         used bytes = %05d\n", usedBytes);
	printf(" largest free block = %05d\n", largestFree);
}
#endif

// Calculate the mb's offset in the buffer
inline uint16_t heapGetOffset(HEAPHANDLE hHeap, MEMORYBLOCK mb)
{
	return (uint16_t)((uint8_t *)mb - (uint8_t *)hHeap);
}

// Return the next block
inline MEMORYBLOCK heapGetNextAddress(HEAPHANDLE hHead, MEMORYBLOCK mb)
{
	return mb->next != CHAIN_END
		? (MEMORYBLOCK)((uint8_t *)hHead + mb->next) 
		: NULL;
}

// Return the previous block
inline MEMORYBLOCK heapGetPreviousAddress(HEAPHANDLE hHead, MEMORYBLOCK mb)
{
	return mb->previous != CHAIN_END
		? (MEMORYBLOCK)((uint8_t*)hHead + mb->previous)
		: NULL;
}

// Returns the memory block for the specified address
inline MEMORYBLOCK heapGetMB(uint8_t *address)
{
	return (MEMORYBLOCK)(address - sizeof(MEMORYBLOCKSTRUCT));
}

// Returns a pointer to the data referenced by the offset
inline uint8_t* heapGetData(MEMORYBLOCK mb)
{
	return (uint8_t*)((uint8_t*)mb + sizeof(MEMORYBLOCKSTRUCT));
}

// Returns the free list head
inline MEMORYBLOCK heapGetFreeList(HEAPHANDLE hHeap)
{
	return &(hHeap->freeList);
}

// Returns the used list head
inline MEMORYBLOCK heapGetUsedList(HEAPHANDLE hHeap)
{
	return &(hHeap->usedList);
}

// Insert newItem after target
void heapInsertAfter(HEAPHANDLE hHeap, MEMORYBLOCK target, MEMORYBLOCK newItem)
{
	newItem->next = target->next;
	newItem->previous = heapGetOffset(hHeap, target);
	target->next = heapGetOffset(hHeap, newItem);

	if (newItem->next != CHAIN_END)
		heapGetNextAddress(hHeap, newItem)->previous = heapGetOffset(hHeap, newItem);
}

// Remove mb from the list within which it resides
void heapRemoveFromList(HEAPHANDLE hHeap, MEMORYBLOCK mb)
{
	heapGetPreviousAddress(hHeap, mb)->next = mb->next;

	if (mb->next != CHAIN_END)
		heapGetNextAddress(hHeap, mb)->previous = mb->previous;
}

// Returns:
// -1 if first is adjacent and before second
// 1 if second is adjacent and before first
// 0 if they are not adacent to each other
int heapGetIsAdjacent(MEMORYBLOCK first, MEMORYBLOCK second)
{
	if (heapGetData(first) + first->length == (uint8_t *)second)
		return -1;
	else if (heapGetData(second) + second->length == (uint8_t*)first)
		return 1;
	else
		return 0;
}
