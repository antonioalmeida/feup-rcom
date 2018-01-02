#include "parser.h"

#define BASE_INDEX_AFTER_PROTOCOL_IDENTIFIER 6

int parseURL(URLInfo* url, const char* urlStr) {
	// Building regular expression to check if given URL is valid
	const char* regularExpression = "ftp://([a-z0-9]*:[a-z0-9]*@)?[a-z0-9.~-]+/[a-z0-9/~._-]+";
	regex_t* regex = (regex_t*) malloc(sizeof(regex_t));
	size_t nmatch = strlen(urlStr);
	regmatch_t pmatch[nmatch];

	if (regcomp(regex, regularExpression, REG_EXTENDED | REG_ICASE) != 0) {
		perror("Bad regex");
		return -1;
	}

	if (regexec(regex, urlStr, nmatch, pmatch, REG_EXTENDED | REG_ICASE) != 0) {
		perror("Invalid URL format");
		return -1;
	}

	free(regex);

	int credentialsGiven = 0;
	if (memchr(urlStr, '@', strlen(urlStr))) //If '@' is present, it is because username and password were provided
		credentialsGiven = 1;

	int cumulativeLength = BASE_INDEX_AFTER_PROTOCOL_IDENTIFIER;
	int substringLength = 0;

	if (credentialsGiven) {
		//Retrieve username
		getSubstring(urlStr, cumulativeLength, ':', &substringLength);
		memcpy(url->user, urlStr+cumulativeLength, substringLength);

		//printf("Got username, is %s\n", url->user);

		cumulativeLength += substringLength + 1;

		//Retrieve password
		getSubstring(urlStr, cumulativeLength, '@', &substringLength);
		memcpy(url->password, urlStr+cumulativeLength, substringLength);

		//printf("Got password, is %s\n", url->password);

		cumulativeLength += substringLength + 1;
	}

	//Retrieve host
	getSubstring(urlStr, cumulativeLength, '/', &substringLength);
	memcpy(url->host, urlStr+cumulativeLength, substringLength);

	cumulativeLength += substringLength + 1;

	//printf("Got host, is %s\n", url->host);

	char* lastSlash = strrchr(urlStr, '/');
	int pathSize = lastSlash - (urlStr + cumulativeLength);

	if(pathSize > 0){
		memcpy(url->path, urlStr+cumulativeLength, pathSize);
		cumulativeLength += pathSize + 1;
	}

	//printf("Got path, is %s\n", url->path);

	//Retrieve filename
	int filenameSize = strlen(urlStr) - cumulativeLength;
	memcpy(url->filename, urlStr+cumulativeLength, filenameSize);

	//printf("Got filename, is %s\n", url->filename);

	return 0;
}

int getIpByHost(URLInfo* url) {
	struct hostent* h;

	if ((h = gethostbyname(url->host)) == NULL) {
		herror("gethostbyname");
		return -1;
	}

	//	printf("Host name  : %s\n", h->h_name);
	//	printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));

	char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
	strcpy(url->ip, ip);

	return 0;
}

int getSubstring(const char* str, int start, char delimiter, int* substringLength) {
	int length = strlen(str+start);
	char* end = memchr(str+start, delimiter, length);
	if(end == NULL)
		return -1;

	*substringLength = end - (str + start);
	return 0;
}
