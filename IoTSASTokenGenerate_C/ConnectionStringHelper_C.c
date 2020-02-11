#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef _DEBUG
#include <stdio.h>
#endif
#include "sha256.h"
#include "ConnectionStringHelper_C.h"

//#define _TESTING

static void dumpBuffer(uint8_t* buffer, size_t bufferLength);

CONNECTIONSTRINGHANDLE CreateConnectionStringHandle(const char* connectionString)
{
	if (connectionString == NULL)
		return NULL;

	int itemCount = 0;
	int index = 0;
	int newIndex = 0;
	int eqIndex = 0;
	size_t csLen = strlen(connectionString);
	int tokenCount = 0;
	const char* keywordStart;
	const char* valueStart;
	const char* valueEnd = connectionString;
	CONNECTIONSTRINGHANDLE h = NULL;
	bool errorFound = false;

	while (NULL != (valueEnd = strchr(valueEnd, ';')))
	{
		tokenCount++;
		valueEnd++;
	}

	h = malloc(sizeof(CONNECTIONSTRINGSTRUCT));

	if (h == NULL)
		return h;

	memset(h, 0, sizeof(*h));
	h->tokenCount = ++tokenCount;
	h->keywords = malloc(sizeof(char*) * tokenCount);
	h->values = malloc(sizeof(char*) * tokenCount);

	if (h->keywords == NULL || h->values == NULL)
		errorFound = true;

	if (!errorFound)
	{
		memset(h->keywords, 0, sizeof(char*) * tokenCount);
		memset(h->values, 0, sizeof(char*) * tokenCount);
		tokenCount = 0;

		keywordStart = valueEnd = connectionString;

		bool run = true;

		while (run)
		{
			valueStart = strchr(keywordStart, '=');

			if (valueStart == NULL)
			{
				errorFound = true;
				break;
			}

			h->keywords[tokenCount] = (char*)malloc(++valueStart - keywordStart);
			h->keywords[tokenCount][valueStart - keywordStart - 1] = '\0';

			for (int i = 0; i < valueStart - keywordStart - 1; i++)
			{
				(h->keywords[tokenCount][i]) = tolower(keywordStart[i]);
			}

			valueEnd = strchr(++valueEnd, ';');

			if (valueEnd == NULL)
			{
				valueEnd = connectionString + strlen(connectionString);
				run = false;
			}

			h->values[tokenCount] = (char*)malloc(valueEnd - valueStart + 1);
			h->values[tokenCount][valueEnd - valueStart] = '\0';
			memcpy(h->values[tokenCount], valueStart, valueEnd - valueStart);
			tokenCount++;

			if (*valueEnd)
				keywordStart = 1 + valueEnd;
		}
	}

	if (errorFound)
	{
		for (int i = 0; i < h->tokenCount; i++)
		{
			free(h->keywords[i]);
			free(h->values[i]);
		}

		free(h);
		h = NULL;
	}

#ifdef _DEBUG
	if (h)
	{
		printf("Found %d tokens\r\n", h->tokenCount);

		for (int i = 0; i < h->tokenCount; i++)
		{
			printf("keyword=%s;value=%s\r\n", h->keywords[i], h->values[i]);
		}
	}
#endif

	return h;
}

int DestroyConnectionStringHandle(CONNECTIONSTRINGHANDLE h)
{
	if (h != NULL)
	{
		for (int i = 0; i < h->tokenCount; i++)
		{
			free(h->keywords[i]);
			free(h->values[i]);
		}

		free(h);
	}

	return 0;
}

// Return the value for a keyword in the connection string
const char* GetKeywordValue(CONNECTIONSTRINGHANDLE h, const char* keyword)
{
	char* work = (char*)malloc(strlen(keyword) + 1);

	if (work == NULL)
		return NULL;

	const char* p = keyword;
	char* d = work;

	for (; *p; p++)
	{
		*d++ = tolower(*p);
	}

	*d = '\0';

	int i;

	for (i = 0; i < h->tokenCount; i++)
	{
		if (0 == strcmp(work, h->keywords[i]))
			break;
	}

	free(work);

	return (i < h->tokenCount)
		? h->values[i]
		: NULL;
}

// Encode a URL
int urlEncode(const char* urlIn, char* urlOut, int urlOutLen)
{
	static const char *hex = "0123456789ABCDEF";
	static const char *specials = "-._";

	int count = 0;
  
	for (int i = 0; i < strlen(urlIn); i++)
	{
		if (('a' <= urlIn[i] && urlIn[i] <= 'z') ||
			('A' <= urlIn[i] && urlIn[i] <= 'Z') ||
			('0' <= urlIn[i] && urlIn[i] <= '9') ||
			(NULL != strchr(specials, urlIn[i])))
		{
			if (urlOut != NULL && count < urlOutLen)
				*(urlOut + count) = urlIn[i];
				
			++count;
		}
		else
		{
			if (urlOut != NULL && count + 2 < urlOutLen)
			{
				urlOut[count] = '%';
				urlOut[count + 1] = hex[urlIn[i] >> 4];
				urlOut[count + 2] = hex[urlIn[i] & 15];
			}

			count += 3;
		}
	}

	if (urlOut != NULL && count < urlOutLen)
		urlOut[count] = '\0';

	count++;

	return count;
}

// Encodes the input into Base64
int encodeBase64(const char *input, int inputLength, char *output, int outputLength)
{
	const char* CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

	char b;
	int counter = 0;

	for (int i = 0; i < inputLength; i += 3)
	{
		b = (input[i] & 0xfc) >> 2;

		if (output != NULL && counter < outputLength)
			output[counter] = CODES[b];

		counter++;

		b = (input[i] & 0x03) << 4;
    
		if (i + 1 < inputLength)      
		{
			b |= (input[i + 1] & 0xF0) >> 4;


			if (output != NULL && counter < outputLength)
				output[counter] = CODES[b];

			counter++;

			b = (input[i + 1] & 0x0F) << 2;
      
			if (i + 2 < inputLength)  
			{
				b |= (input[i + 2] & 0xC0) >> 6;


				if (output != NULL && counter < outputLength)
					output[counter] = CODES[b];

				counter++;

				b = input[i + 2] & 0x3F;

				if (output != NULL && counter < outputLength)
					output[counter] = CODES[b];

				counter++;
			} 
			else  
			{
				if (output != NULL && counter < outputLength)
					output[counter] = CODES[b];

				counter++;

				if (output != NULL && counter < outputLength)
					output[counter] = ('=');

				counter++;
			}
		} 
		else      
		{
			if (output != NULL && counter < outputLength)
				output[counter] = CODES[b];

			counter++;

			if (output != NULL && counter < outputLength)
			{
				output[counter] = '=';
				output[counter + 1] = '=';
			}

			counter += 2;
		}    
	}

	if (output != NULL && counter < outputLength)
		output[counter] = '\0';

	return ++counter;
}

// Decodes from Base64
int decodeBase64(const char* input, char* output, int outputLength)
{
	static const char* CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

	size_t b[4];
	int inputLen = (int)strlen(input);

	if (strlen(input) % 4 != 0)
		return -1;    // Base64 string's length must be a multiple of 4
    
	int requiredLen = (int)((inputLen * 3) / 4 - (strchr(input, '=') != NULL? (inputLen - (strchr(input, '=') - input)) : 0));

	if (outputLength == 0 || output == NULL)
		return requiredLen;

	if (requiredLen > outputLength)
		return -2;    // Output buffer is too short

	int j = 0;

	for (size_t i = 0; i < inputLen; i += 4)
	{
		b[0] = strchr(CODES, input[i]) - CODES;
		b[1] = strchr(CODES, input[i + 1]) - CODES;
		b[2] = strchr(CODES, input[i + 2]) - CODES;
		b[3] = strchr(CODES, input[i + 3]) - CODES;

		output[j++] = (uint8_t)(((b[0] << 2) | (b[1] >> 4)));

		if (b[2] < 64)
		{
			output[j++] = (uint8_t)(((b[1] << 4) | (b[2] >> 2)));
      
			if (b[3] < 64)  
			{
				output[j++] = (uint8_t)(((b[2] << 6) | b[3]));
			}
		}
	}

	return requiredLen;
}

// Returns the hashed value of the data and the key
static int hashIt(const char* data, unsigned char* key, int keyLength, char* output, int outputLen)
{
	uint8_t signedOut[32];

	generateHash(signedOut, (uint8_t *)data, strlen(data), key, keyLength);

	char* inBase64;
	int inBase64Len;

	inBase64Len = encodeBase64(signedOut, sizeof(signedOut), NULL, 0);
	inBase64 = (char*)malloc(inBase64Len);

	if (inBase64 == NULL)
		return -1;

	inBase64Len = encodeBase64(signedOut, sizeof(signedOut), inBase64, inBase64Len);

	int result = urlEncode(inBase64, output, outputLen);

	free(inBase64);

	return result;
}

// Generate the SAS token for the IoT Hub
int generatePassword(CONNECTIONSTRINGHANDLE h, long tokenTTL, char* output, int outputLen)
{
	char* uri;
	char* encodedUri;
#ifdef _TESTING
	int32_t epoch = 0;
#else
	int32_t epoch = (int32_t) time(0);
#endif
	int32_t tokenExpiry = epoch + tokenTTL;
	char tokenExpiryStr[15];

	if ((snprintf(tokenExpiryStr, sizeof(tokenExpiryStr), "%d", tokenExpiry)) > sizeof(tokenExpiryStr))
		return -1;

	int uriLen = 1 + strlen(GetKeywordValue(h, "hostname")) + strlen("/devices/") + strlen(GetKeywordValue(h, "deviceid"));
	int encodedUriLen;

	uri = (char*)malloc(uriLen);

	if (uri == NULL)
		return -1;

	sprintf_s(uri, uriLen, "%s/devices/%s", GetKeywordValue(h, "hostname"), GetKeywordValue(h, "deviceid"));

#ifdef _DEBUG
	printf("URL to encode >%s<\r\n", uri);
#endif

	encodedUriLen = urlEncode(uri, NULL, 0);
	encodedUri = (char*)malloc(encodedUriLen);

	if (encodedUri == NULL)
		return -1;

	urlEncode(uri, encodedUri, encodedUriLen);

#ifdef _DEBUG
	printf("URL encoded >%s<\r\n\n", encodedUri);
#endif

	free(uri);

	int toSignLen = strlen(encodedUri) + 2 + strlen(tokenExpiryStr) + 1;
	char* toSign = (char*)malloc(toSignLen);

	if (toSign == NULL)
		return -1;

	sprintf_s(toSign, toSignLen, "%s\n%s", encodedUri, tokenExpiryStr);

	char* key;
	size_t keyLen;

	keyLen = decodeBase64(GetKeywordValue(h, "SharedAccessKey"), NULL, 0);
	key = (char*)malloc(keyLen);
	keyLen = decodeBase64(GetKeywordValue(h, "SharedAccessKey"), key, keyLen);

#ifdef _DEBUG
	printf("Decoded SharedAccessKey\r\n");
	dumpBuffer(key, keyLen);
	printf("\r\n");
#endif

	char* password;
	int passwordLen;

	passwordLen = hashIt(toSign, key, keyLen, NULL, 0);
	password = (char*)malloc(passwordLen);

	if (password == NULL)
		return -1;

	hashIt(toSign, key, keyLen, password, passwordLen);
	free(key);

	int resultLen = strlen("SharedAccessSignature sr=") + strlen(encodedUri) + strlen("&sig=") + strlen(password) + strlen("&se=") + strlen(tokenExpiryStr) + 1;

	if (output != NULL && outputLen >= resultLen)
	{
		if ((sprintf_s(output, outputLen, "SharedAccessSignature sr=%s&sig=%s&se=%s", encodedUri, password, tokenExpiryStr)) == -1)
			return -1;	// This should never happen
	}

	free(encodedUri);

	return resultLen;
}

////
//// Private method - Build keyword value lookup map
//int ConnectionStringHelper::findTokens(const std::string connectionString)
//{
//	int itemCount = 0;
//	size_t index = 0;
//	size_t newIndex = 0;
//	size_t eqIndex = 0;
//
//	while (index < connectionString.length())
//	{
//		newIndex = connectionString.find(';', index);
//
//		if (newIndex == string::npos)
//			newIndex = connectionString.length();
//
//		eqIndex = connectionString.find('=', index);
//
//		if (eqIndex == string::npos)
//			return 0;
//
//		itemCount++;
//		std::string keyword = connectionString.substr(index, eqIndex - index);
//		std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
//		keyValue.insert(std::pair<const std::string, const std::string>(keyword, connectionString.substr(eqIndex + 1, newIndex - (eqIndex + 1))));
//		index = newIndex + 1;
//	}
//
//	return itemCount;
//}

#ifdef _DEBUG
// Dumps the buffer in hex and character
static void dumpBuffer(uint8_t *buffer, size_t bufferLength)
{
	for (uint8_t i = 0; i < bufferLength; i += 16)
	{
		printf("%08x  ", i);

		int j;
		uint8_t chunk = (uint8_t)((bufferLength - i < 16) ? bufferLength - i : 16);

		for (j = i; j < i + chunk; j++)
		{
			printf(" %02x", buffer[j]);
		}

		printf("  ");

		if (chunk < 16)
		{
			for (j = chunk; j < 16; j++)
				printf("   ");
		}

		for (j = i; j < i + chunk; j++)
		{
			printf("%c", (isprint(buffer[j])) ? buffer[j] : '.');
		}

		printf("\r\n");
	}
}
#endif

