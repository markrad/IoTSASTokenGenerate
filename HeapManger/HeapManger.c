// HeapManger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../IoTSASTokenGenerateNoMalloc/heap.h"

int main()
{
	printf("Starting heap test\r\n\n");

	uint8_t buffer[8192];
	HEAPHANDLE h = heapInit(buffer, sizeof(buffer));

	void* p1 = heapMalloc(h, 40);
	memset(p1, 0x01, 40);
	void* p2 = heapMalloc(h, 60);
	memset(p2, 0x02, 60);
	void* p3 = heapMalloc(h, 10);
	memset(p3, 0x03, 10);
	void* p4 = heapMalloc(h, 32);
	memset(p4, 0x04, 32);
	heapFree(h, p2);
	heapFree(h, p1);
	heapFree(h, p4);
	heapFree(h, p3);
	p1 = heapMalloc(h, 4000);
	p2 = heapMalloc(h, 80);
	heapFree(h, p1);
	heapFree(h, p2);

	p1 = heapMalloc(h, 100);
	p2 = heapMalloc(h, 200);
	p3 = heapMalloc(h, 200);
	heapRealloc(h, p2, 100);
	heapFree(h, p2);
	heapRealloc(h, p1, 20);
	heapRealloc(h, p3, 50);
	heapFree(h, p1);
	heapFree(h, p3);
	p1 = heapMalloc(h, 200);
	p2 = heapMalloc(h, 20);

	for (int i = 199; i > 180; i--)
		heapRealloc(h, p1, i);

	heapFree(h, p1);
	heapFree(h, p2);

	p1 = heapMalloc(h, 11);
	p2 = heapMalloc(h, 4);

	strcpy(p1, "0123456789");
	printf("%s\n", (char*)p1);
	p1 = heapRealloc(h, p1, 21);
	printf("%s\n", (char*)p1);
	strcat(p1, "9876543210");
	printf("%s\n", (char*)p1);
	p1 = heapRealloc(h, p1, 31);
	printf("%s\n", (char*)p1);
	strcat(p1, "9876543210");
	printf("%s\n", (char*)p1);

	heapFree(h, p2);
	heapFree(h, p1);

	void* ptrs[100];
	srand((unsigned)time(NULL));

	for (int i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++)
	{
		p1 = heapMalloc(h, (size_t)(rand() % 32) + 1);
		size_t len = (size_t)(rand() % 256) + 1;
		ptrs[i] = heapMalloc(h, len);

		if (ptrs[i] != NULL)
		{
			printf("allocated %d bytes\n", (int)len);
			memset(ptrs[i], 'M', len);
		}

		heapFree(h, p1);
	}

	int freedCount = 0;

	for (int i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++)
	{
		if (ptrs[i] != NULL)
		{
			freedCount++;
			heapFree(h, ptrs[i]);
			ptrs[i] = NULL;
			printf("freed %d\n", freedCount);
		}
	}

	printf("Done\n");
}

