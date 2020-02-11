// IoTSASTokenGenerate_C.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <stdio.h>
#include <stdlib.h>
#include "ConnectionStringHelper_C.h"

int usage();

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Missing or invalid arguments\r\n\n");
		return usage();
	}

	CONNECTIONSTRINGHANDLE csh = CreateConnectionStringHandle(*(++argv));

	if (csh == NULL)
		return 4;

	printf("HostName = %s\r\n", GetKeywordValue(csh, "Hostname"));
	printf("DeviceId = %s\r\n", GetKeywordValue(csh, "DEVICEID"));
	printf("SharedAccessKey = %s\r\n", GetKeywordValue(csh, "SharedAccessKey"));

	int count = urlEncode("https://markradhub2.azure-devices.net/JavaTest1/", NULL, 0);

	char* test = (char*)malloc(count);

	int count2 = urlEncode("https://markradhub2.azure-devices.net/JavaTest1/", test, count);

	printf("%s\r\n", test);

	free(test);

	char* input = "Now is the time for all good men to come to the aid of the party";

	count = encodeBase64(input, strlen(input), NULL, 0);
	test = (char*)malloc(count);
	count2 = encodeBase64(input, strlen(input), test, count);

	printf("%s\r\n", test);

	count = strlen(input);
	char* decode = (char*)malloc(count + 1);
	count2 = decodeBase64(test, decode, count);
	decode[count] = '\0';
	printf("%s\r\n", decode);
	free(decode);
	free(test);

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
