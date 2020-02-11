#include "stdafx.h"

#include <algorithm>
#include <time.h>

#include "sha256.h"
#include "ConnectionStringHelper.h"

//#define _TESTING

const std::string ConnectionStringHelper::CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

/*
 * Constructor
 * 
 *  connectionString          Azure IoT hub device connection string
 */
ConnectionStringHelper::ConnectionStringHelper(const std::string connectionString)
{
	_tokenCount = findTokens(connectionString);
}

/*
 * Destructor
 */
ConnectionStringHelper::~ConnectionStringHelper()
{
}

/*
 * getKeywordValue: Returns the value from the connection string for
 * the specified keyword.
 * 
 *  keyword:          Self explanatory
 *  
 *  Returns the keyword value or and empty string if not found
 */
const std::string ConnectionStringHelper::getKeywordValue(const std::string keywordIn)
{
	std::string keyword = keywordIn;

	std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);

	if (keyValue.find(keyword) == keyValue.end())
		return "";
	else
		return keyValue[keyword];
}

//
// Encode string for URL
string ConnectionStringHelper::urlEncode(const string url)
{
	static const char *hex = "0123456789ABCDEF";
	static const string specials = "-._";
  
	string result = "";

	for (uint8_t i = 0; i < url.length(); i++)
	{
		if (('a' <= url[i] && url[i] <= 'z') ||
			('A' <= url[i] && url[i] <= 'Z') ||
			('0' <= url[i] && url[i] <= '9') ||
			(string::npos != specials.find(url[i], 0)))
		{
			result += url[i];
		}
		else
		{
			result += '%';
			result += hex[url[i] >> 4];
			result += hex[url[i] & 15];
		}
	}

  return result;
}


//
// Encodes the input into Base64
string ConnectionStringHelper::encodeBase64(const uint8_t *input, int inputLength)
{
	string result = "";

	int8_t b;

	for (int i = 0; i < inputLength; i += 3)
	{
		b = (input[i] & 0xfc) >> 2;
		result += CODES[b];
		b = (input[i] & 0x03) << 4;
    
		if (i + 1 < inputLength)      
		{
			b |= (input[i + 1] & 0xF0) >> 4;
			result += CODES[b];
			b = (input[i + 1] & 0x0F) << 2;
      
			if (i + 2 < inputLength)  
			{
				b |= (input[i + 2] & 0xC0) >> 6;
				result += CODES[b];
				b = input[i + 2] & 0x3F;
				result += CODES[b];
			} 
			else  
			{
				result += CODES[b];
				result += ('=');
			}
		} 
		else      
		{
			result += CODES[b];
			result += ("==");
		}    
	}
	return result;
}


//
// Decodes from Base64
size_t ConnectionStringHelper::decodeBase64(const string input, uint8_t *output, size_t outputLength)
{
	size_t b[4];

	if (input.length() % 4 != 0)
		return -1;    // Base64 string's length must be a multiple of 4
    
	size_t requiredLen = (input.length() * 3) / 4 - (input.find('=') != string::npos ? (input.length() - input.find('=')) : 0);

	if (outputLength == 0 || output == NULL)
		return requiredLen;

	if (requiredLen > outputLength)
		return -2;    // Output buffer is too short

	int j = 0;

	for (size_t i = 0; i < input.length(); i += 4)
	{
		b[0] = CODES.find(input[i]);
		b[1] = CODES.find(input[i + 1]);
		b[2] = CODES.find(input[i + 2]);
		b[3] = CODES.find(input[i + 3]);

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


//
// Returns the hashed value of the data and the key
string ConnectionStringHelper::hashIt(const string data, uint8_t *key, size_t keyLength)
{
	uint8_t signedOut[32];
	string work;

	generateHash(signedOut, (uint8_t *)data.c_str(), (size_t) data.length(), key, keyLength);
	work = encodeBase64(signedOut, sizeof(signedOut));
	return urlEncode(work);
}

//
// Generate the SAS token for the IoT Hub
string ConnectionStringHelper::generatePassword(int32_t tokenTTL)
{
	string uri;
#ifdef _TESTING
	int32_t epoch = 0;
#else
	int32_t epoch = (int32_t)time(0);
#endif
	int32_t tokenExpiry = epoch + tokenTTL;

#ifdef _DEBUG
	printf("URL to encode >%s<\r\n", (getKeywordValue("hostname") + "/devices/" + getKeywordValue("deviceid")).c_str());
#endif

	uri = urlEncode(getKeywordValue("hostname") + "/devices/" + getKeywordValue("deviceid"));

#ifdef _DEBUG
	printf("URL encoded >%s<\r\n\n", uri.c_str());
#endif

	string toSign = uri + "\n" + to_string(tokenExpiry);
	uint8_t *key;
	size_t keyLen;

	keyLen = decodeBase64(getKeywordValue("SharedAccessKey"), NULL, 0);
	key = new uint8_t[keyLen];
	keyLen = decodeBase64(getKeywordValue("SharedAccessKey"), key, keyLen);

#ifdef _DEBUG
	printf("Decoded SharedAccessKey\r\n");
	dumpBuffer(key, keyLen);
	printf("\r\n");
#endif

	string password = hashIt(toSign, key, keyLen);

	delete [] key;

	string result;
  
	result = string("SharedAccessSignature sr=") + uri + "&sig=" + password + "&se=" + to_string(tokenExpiry);

	return result;
}

//
// Private method - Build keyword value lookup map
int ConnectionStringHelper::findTokens(const std::string connectionString)
{
	int itemCount = 0;
	size_t index = 0;
	size_t newIndex = 0;
	size_t eqIndex = 0;

	while (index < connectionString.length())
	{
		newIndex = connectionString.find(';', index);

		if (newIndex == string::npos)
			newIndex = connectionString.length();

		eqIndex = connectionString.find('=', index);

		if (eqIndex == string::npos)
			return 0;

		itemCount++;
		std::string keyword = connectionString.substr(index, eqIndex - index);
		std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
		keyValue.insert(std::pair<const std::string, const std::string>(keyword, connectionString.substr(eqIndex + 1, newIndex - (eqIndex + 1))));
		index = newIndex + 1;
	}

	return itemCount;
}
#ifdef _DEBUG
//
// Dumps the buffer in hex and character
void ConnectionStringHelper::dumpBuffer(uint8_t *buffer, size_t bufferLength)
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

