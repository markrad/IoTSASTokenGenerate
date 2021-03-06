// IoTSASTokenGenerate.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ConnectionStringHelper.h"

int usage();

int main(int argc, char **argv)
{
	const char *keywords[] = { "HostName", "DeviceId", "SharedAccessKey" };
	const int HostName = 0;
	const int DeviceId = 1;
	const int SharedAccessKey = 2;
	
	if (argc != 2)
	{
		printf("Missing or invalid arguments\r\n\n");
		return usage();
	}

	ConnectionStringHelper csh = ConnectionStringHelper(*(++argv));

	if (csh.tokenCount() != 3)
	{
		printf("Connection string does not contain the correct number of tokens\r\n\n");
		return usage();
	}

	printf("Connection string details:\r\n\n");

	for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
	{
		string value = csh.getKeywordValue(keywords[i]);
		if (value == "")
		{
			printf("Connection string is missing token %s\r\n\n", keywords[i]);
			return usage();
		}
		else
		{
			printf("Keyword: %s\n  Value: %s\r\n\n", keywords[i], value.c_str());
		}
	}

	string MQTTClientId = csh.getKeywordValue(keywords[DeviceId]);
	string MQTTUserName = csh.getKeywordValue(keywords[HostName]) + "/" + csh.getKeywordValue(keywords[DeviceId]);
	string MQTTServer = csh.getKeywordValue(keywords[HostName]);
	string MQTTPassword = csh.generatePassword(3600);

	printf("MQTTClientId=%s\r\n", MQTTClientId.c_str());
	printf("MQTTUserName=%s\r\n", MQTTUserName.c_str());
	printf("MQTTServer=%s\r\n", MQTTServer.c_str());
	printf("MQTTPassword=%s\r\n", MQTTPassword.c_str());

    return 0;
}

int usage()
{
	printf("Usage: IoTSASTokenGenerate <deviceConnectionString>");

	return 4;
}

