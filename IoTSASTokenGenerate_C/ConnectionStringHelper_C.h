#pragma once

typedef struct _CONNECTIONSTRINGSTRUCT
{
	int tokenCount;
	char** keywords;
	char** values;
} CONNECTIONSTRINGSTRUCT, *CONNECTIONSTRINGHANDLE;

CONNECTIONSTRINGHANDLE CreateConnectionStringHandle(const char* connectionString);
const char* GetKeywordValue(CONNECTIONSTRINGHANDLE, const char* keyword);
int DestroyConnectionStringHandle(CONNECTIONSTRINGHANDLE hcs);

int urlEncode(const char* urlIn, char* urlOut, int urlOutLen);
int encodeBase64(const char* input, int inputLength, char* output, int outputLength);
int decodeBase64(const char* input, char* output, int outputLength);
int generatePassword(CONNECTIONSTRINGHANDLE h, long tokenTTL, char* output, int outputLen);


/*
class ConnectionStringHelper
{
private:
	typedef std::map<std::string, std::string> TKeyValue;
	
	TKeyValue keyValue;
	int _tokenCount;
  
	const static std::string CODES;

	int findTokens(const std::string connectionString);
	string hashIt(const string data, uint8_t *key, size_t keyLength);
#ifdef _DEBUG
	static void dumpBuffer(uint8_t *buffer, size_t bufferLength);
#endif

public:
	static string urlEncode(const string url);
	static string encodeBase64(const uint8_t *input, int inputLength);
	static size_t decodeBase64(const string input, uint8_t *output, size_t outputLength);

	ConnectionStringHelper(const std::string connectionString);
	~ConnectionStringHelper();
	int tokenCount() { return _tokenCount; }
	const std::string getKeywordValue(const std::string keyword);
	string generatePassword(int32_t tokenTTL);
};
*/