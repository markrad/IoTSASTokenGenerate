#pragma once

#include <string>
#include <map>

using namespace std;

class ConnectionStringHelper
{
private:
	typedef std::map<std::string, std::string> TKeyValue;
	
	TKeyValue keyValue;
	int _tokenCount;
  
	const static std::string CODES;

	int findTokens(const std::string connectionString);
	string hashIt(const string data, uint8_t *key, size_t keyLength);
	static void dumpBuffer(uint8_t *buffer, uint32_t bufferLength);

public:
	static string urlEncode(const string url);
	static string encodeBase64(const uint8_t *input, int inputLength);
	static int decodeBase64(const string input, uint8_t *output, uint32_t outputLength);

	ConnectionStringHelper(const std::string connectionString);
	~ConnectionStringHelper();
	int tokenCount() { return _tokenCount; }
	const std::string getKeywordValue(const std::string keyword);
	string generatePassword(int32_t tokenTTL);
};
