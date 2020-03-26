// IoTSASTokenGenerate_C.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <stdlib.h>
#include "ConnectionStringHelper_NoMalloc.h"

int usage();

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Missing or invalid arguments\r\n\n");
		return usage();
	}

	unsigned char buffer[2048];

	CONNECTIONSTRINGHANDLE csh = CreateConnectionStringHandle(*(++argv), buffer, sizeof(buffer));

	if (csh == NULL)
		return 4;

	printf("HostName = %s\r\n", GetKeywordValue(csh, "Hostname"));
	printf("DeviceId = %s\r\n", GetKeywordValue(csh, "DEVICEID"));
	printf("SharedAccessKey = %s\r\n", GetKeywordValue(csh, "SharedAccessKey"));

	char* password = NULL;
	int passwordLen;
	int saveLen = 0;

	// SAS tokens are not a fixed size. It is possible that the new SAS token
	// will be longer than the previous estimate. Keep increasing the buffer
	// until it fits.
	while (saveLen < (passwordLen = generatePassword(csh, 3600, password, saveLen)))
	{
		free(password);
		password = (char*)malloc(passwordLen);
		saveLen = passwordLen;
	}

	printf("MQTT client id = %s\r\n", GetKeywordValue(csh, "deviceid"));
	printf("MQTT user name = %s/%s\r\n", GetKeywordValue(csh, "hostname"), GetKeywordValue(csh, "deviceid"));
	printf("MQTT server name = %s\r\n", GetKeywordValue(csh, "hostname"));
	printf("MQTT password = %s\r\n", password);

	free(password);
	DestroyConnectionStringHandle(csh);
}

int usage()
{
	printf("Usage: IoTSASTokenGenerate <deviceConnectionString>");

	return 4;
}
