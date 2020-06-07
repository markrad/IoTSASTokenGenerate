#include "heap.h"

#ifdef _DEBUG_HEAP
#include <stdio.h>

#define _DEBUG_HEAP_SANITY(HEAP) (heapSanity(HEAP));
#else
#define _DEBUG_HEAP_SANITY(HEAP) (0)
#endif

#include <memory.h>

#define MIN_ALLOC sizeof(MEMORYBLOCKSTRUCT) + 4
#define CHAIN_END UINT16_MAX
#define MIN_BUFFER 1024
#define MAX_BUFFER UINT16_MAX

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
} HHEAPSTRUCT, * HHEAP;

MEMORYBLOCK heapGetFreeList(HEAPHANDLE hHeap);
MEMORYBLOCK heapGetUsedList(HEAPHANDLE hHeap);
uint16_t heapGetOffset(HEAPHANDLE hHeap, MEMORYBLOCK mb);
MEMORYBLOCK heapGetNextAddress(HEAPHANDLE hHead, MEMORYBLOCK mb);
MEMORYBLOCK heapGetPreviousAddress(HEAPHANDLE hHead, MEMORYBLOCK mb);
MEMORYBLOCK heapGetMB(uint8_t* address);
uint8_t* heapGetData(MEMORYBLOCK mb);
void heapInsertIntoFreeList(HEAPHANDLE hHead, MEMORYBLOCK mb);
void heapInsertAfter(HEAPHANDLE hHeap, MEMORYBLOCK target, MEMORYBLOCK newItem);
void heapRemoveFromList(HEAPHANDLE hHeap, MEMORYBLOCK mb);
int heapGetIsAdjacent(MEMORYBLOCK first, MEMORYBLOCK second);
static void* heapTruncate(HEAPHANDLE hHeap, void* address, uint16_t newLength);
static void* heapExtend(HEAPHANDLE hHeap, void* address, uint16_t newLength);

// Initialize the heap structures
HEAPHANDLE heapInit(uint8_t *buffer, size_t bufferLen)
{
	if (buffer == NULL || bufferLen < MIN_BUFFER || bufferLen > MAX_BUFFER)
		return NULL;

#ifdef _DEBUG_HEAP
	memset(buffer, 0xee, bufferLen);
#endif

	HHEAP hHeap = (HHEAP)buffer;

	hHeap->usedList.length = 0;
	hHeap->usedList.next = CHAIN_END;
	hHeap->usedList.previous = CHAIN_END;
	hHeap->freeList.length = 1;
	hHeap->freeList.next = (uint16_t)sizeof(HHEAPSTRUCT);
	hHeap->freeList.previous = CHAIN_END;

	MEMORYBLOCK first = heapGetNextAddress(hHeap, &hHeap->freeList);

	first->length = (uint16_t)(bufferLen - sizeof(HHEAPSTRUCT) - sizeof(MEMORYBLOCKSTRUCT));
	first->next = CHAIN_END;
	first->previous = (uint16_t)((uint8_t*)&hHeap->freeList - buffer);

	_DEBUG_HEAP_SANITY(hHeap);

	return (HEAPHANDLE)hHeap;
}

// Allocate a block
void* heapMalloc(HEAPHANDLE hHeap, size_t bytes)
{
	void* result = NULL;

	if (hHeap != NULL && bytes != 0)
	{
		if (bytes % 2 != 0)
			bytes += 1;

		if (bytes < MIN_ALLOC - sizeof(MEMORYBLOCKSTRUCT))
			bytes = MIN_ALLOC - sizeof(MEMORYBLOCKSTRUCT);

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

			result = heapGetData(mb);
		}
	}

	_DEBUG_HEAP_SANITY(hHeap);

	return result;
}

// Free an allocated block
void heapFree(HEAPHANDLE hHeap, void* address)
{
	if (hHeap != NULL && address != NULL)
	{
		MEMORYBLOCK mb = heapGetMB(address);

		heapRemoveFromList(hHeap, mb);

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

		heapInsertIntoFreeList(hHeap, mb);
	}

	_DEBUG_HEAP_SANITY(hHeap);
}

void* heapRealloc(HEAPHANDLE hHeap, void* address, uint16_t newLength)
{
	if (hHeap != NULL && address != NULL)
	{
		MEMORYBLOCK mb = heapGetMB(address);

		return mb->length > newLength
			? heapTruncate(hHeap, address, newLength)
			: mb->length < newLength
			? heapExtend(hHeap, address, newLength)
			: address;
	}
	else
	{
		return NULL;
	}
}

static void* heapTruncate(HEAPHANDLE hHeap, void* address, uint16_t newLength)
{
	void* result = NULL;

	if (hHeap != NULL && address != NULL)
	{
		if (newLength == 0)
		{
			heapFree(hHeap, address);
		}
		else
		{
			if (newLength % 2 != 0)
				newLength += 1;

			MEMORYBLOCK mb = heapGetMB(address);

			if (newLength == mb->length)
			{
				result = address;
			}
			else if (newLength < mb->length)
			{
				result = address;

				MEMORYBLOCK search = heapGetNextAddress(hHeap, heapGetFreeList(hHeap));

				while (search != NULL)
				{
					if (1 == heapGetIsAdjacent(search, mb))
						break;

					search = heapGetNextAddress(hHeap, search);
				}

				if (search != NULL || mb->length - newLength > MIN_ALLOC)
				{
					MEMORYBLOCK trailer = (MEMORYBLOCK)((uint8_t*)address + newLength);
					trailer->length = mb->length - newLength - sizeof(MEMORYBLOCKSTRUCT);
					mb->length = newLength;

					if (search != NULL)
					{
						heapRemoveFromList(hHeap, search);
						trailer->length += (search->length + sizeof(MEMORYBLOCKSTRUCT));
					}

					heapInsertIntoFreeList(hHeap, trailer);
				}
			}
		}
	}

	_DEBUG_HEAP_SANITY(hHeap);

	return result;
}

static void* heapTruncate(HEAPHANDLE hHeap, void* address, uint16_t newLength)
{
	void* result = NULL;

	if (hHeap != NULL && address != NULL)
	{
		if (newLength == 0)
		{
			heapFree(hHeap, address);
		}
		else
		{
			if (newLength % 2 != 0)
				newLength += 1;

			MEMORYBLOCK mb = heapGetMB(address);

			if (newLength == mb->length)
			{
				result = address;
			}
			else if (newLength < mb->length)
			{
				result = address;

				MEMORYBLOCK search = heapGetNextAddress(hHeap, heapGetFreeList(hHeap));

				while (search != NULL)
				{
					if (1 == heapGetIsAdjacent(search, mb))
						break;

					search = heapGetNextAddress(hHeap, search);
				}

				if (search != NULL || mb->length - newLength > MIN_ALLOC)
				{
					MEMORYBLOCK trailer = (MEMORYBLOCK)((uint8_t*)address + newLength);
					trailer->length = mb->length - newLength - sizeof(MEMORYBLOCKSTRUCT);
					mb->length = newLength;

					if (search != NULL)
					{
						heapRemoveFromList(hHeap, search);
						trailer->length += (search->length + sizeof(MEMORYBLOCKSTRUCT));
					}

					heapInsertIntoFreeList(hHeap, trailer);
				}
			}
		}
	}

	_DEBUG_HEAP_SANITY(hHeap);

	return result;
}

static void* heapExtend(HEAPHANDLE hHeap, void* address, uint16_t newLength)
{
	void* result = NULL;

	if (hHeap != NULL && address != NULL)
	{
		if (newLength % 2 != 0)
			newLength += 1;

		MEMORYBLOCK mb = heapGetMB(address);

		if (newLength == mb->length)
		{
			result = address;
		}
		else if (newLength > mb->length)
		{
			MEMORYBLOCK search = heapGetNextAddress(hHeap, heapGetFreeList(hHeap));

			while (search != NULL)
			{
				if (1 == heapGetIsAdjacent(search, mb))
					break;

				search = heapGetNextAddress(hHeap, search);
			}

			if (search != NULL && search->length + sizeof(MEMORYBLOCKSTRUCT) >= newLength - mb->length)
			{
				// Can extend into adjacent free node
				heapRemoveFromList(hHeap, search);

				MEMORYBLOCK newFree = (MEMORYBLOCK)(heapGetData(mb) + newLength);

				newFree->length = search->length - (newLength - mb->length);
				mb->length = newLength;

				if (newFree->length > 0)
				{
					heapInsertIntoFreeList(hHeap, newFree);
					result = address;
				}
			}
			else
			{
				result = heapMalloc(hHeap, newLength);

				if (result != NULL)
				{
					memcpy(result, heapGetData(mb), mb->length);
					heapFree(hHeap, heapGetData(mb));
				}
			}
		}
	}

	_DEBUG_HEAP_SANITY(hHeap);

	return result;
}

void heapGetInfo(HEAPHANDLE hHeap, HEAPINFO *heapInfo)
{
	MEMORYBLOCK mb;

	int freeBytes = 0;
	int usedBytes = 0;
	int totalBytes = 0;
	int largestFree = 0;

	mb = heapGetUsedList(hHeap);
	
	while (NULL != (mb = heapGetNextAddress(hHeap, mb)))
	{
		heapInfo->totalBytes += mb->length + sizeof(MEMORYBLOCKSTRUCT);
		heapInfo->usedBytes += mb->length;
	}

	mb = heapGetFreeList(hHeap);

	while (NULL != (mb = heapGetNextAddress(hHeap, mb)))
	{
		heapInfo->totalBytes += mb->length + sizeof(MEMORYBLOCKSTRUCT);
		heapInfo->freeBytes += mb->length;

		if (mb->length > heapInfo->largestFree)
			heapInfo->largestFree = mb->length;
	}
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

	printf("\nbytes accounted for = %05d\n", (int)(totalBytes + sizeof(HHEAPSTRUCT)));
	printf("         free bytes = %05d\n", freeBytes);
	printf("         used bytes = %05d\n", usedBytes);
	printf(" largest free block = %05d\n", largestFree);
}
#endif

// Insert the block into the free list sorted by length
inline void heapInsertIntoFreeList(HEAPHANDLE hHeap, MEMORYBLOCK mb)
{
	MEMORYBLOCK target = NULL;

	for (target = heapGetFreeList(hHeap);
		NULL != heapGetNextAddress(hHeap, target) && heapGetNextAddress(hHeap, target)->length < mb->length;
		target = heapGetNextAddress(hHeap, target))
	{
	}

	heapInsertAfter(hHeap, target, mb);
}

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
	return &(((HHEAP)hHeap)->freeList);
}

// Returns the used list head
inline MEMORYBLOCK heapGetUsedList(HEAPHANDLE hHeap)
{
	return &(((HHEAP)hHeap)->usedList);
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
