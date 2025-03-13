// Leonardo Frias CSCE463-500 Spring 2025

#include "pch.h"
#include "Socket.h"


Socket::Socket() : sock(INVALID_SOCKET), remote(nullptr), server({ 0 }), buf((char*)malloc(INITIAL_BUF_SIZE * sizeof(char))), allocatedSize(INITIAL_BUF_SIZE), curPos(0)
{

	// open a TCP socket
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		printf("socket() generated error %d\n", WSAGetLastError());
		// Called in deconstructor WSACleanup();
		return;
	}

}

Socket::~Socket() {
	// Close socket and stop W2_32 DLL
	closesocket(sock);
	delete remote;
	WSACleanup();
}

int Socket::DNSLookup(SOCKET sock, hostent *remote, sockaddr_in &server, char* host, char* port, bool &lookupDone, char* &IP_str, bool verify, int mode, int &successLookups, int &passedIPUniqueness) {

	// For second socket that downloads the page
	if (lookupDone) {
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL)
		{
			return 1;
		}
		else {
			// take the first IP address and copy into sin_addr
			lookupDone = true;
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
		}
		return 0;
	}

	if (verify) {
		if (mode == 1 || mode == 2) {
			printf("\t  Checking host uniqueness... ");
		}
		// first assume that the string is an IP address
		if (seenHosts.find(std::string(host)) != seenHosts.end()) {
			hostsCount++;
			if (mode == 1 || mode == 2) {
				printf("failed.\n");
			}
			return 1;
		}
		if (mode == 1 || mode == 2) {
			printf("passed.\n");
		}
	}


	DWORD IP = inet_addr(host);
	if (IP == INADDR_NONE)
	{
		if (mode == 1 || mode == 2) {
			printf("\t  Doing DNS... ");
		}
		lookupDone = true;
		clock_t start = clock();
		// if not a valid IP, then do a DNS lookup
		if ((remote = gethostbyname(host)) == NULL)
		{
			if (mode == 1 || mode == 2) {
				printf("failed with %d\n", WSAGetLastError());
			}
			return 1;
		}
		else {
			// take the first IP address and copy into sin_addr
			successLookups++;
			memcpy((char*)&(server.sin_addr), remote->h_addr, remote->h_length);
			clock_t duration = clock() - start;
			if (mode == 1 || mode == 2) {
				printf("done in %d ms, found %s\n", duration, inet_ntoa(server.sin_addr));
			}

			if (verify) {
				if (mode == 1 || mode == 2) {
					printf("\t  Checking IP uniqueness... ");
				}
				IP_str = _strdup(inet_ntoa(server.sin_addr));
				if (seenIPs.find(std::string(IP_str)) != seenIPs.end()) {
					ipsCount++;
					if (mode == 1 || mode == 2) {
						printf("failed.\n");
					}
					return 1;
				}
				passedIPUniqueness++;
				if (mode == 1 || mode == 2) {
					printf("passed.\n");
				}
			}
		}

	}
	else
	{
		// if a valid IP, directly drop its binary version into sin_addr
		server.sin_addr.S_un.S_addr = IP;
		IP_str = _strdup(inet_ntoa(server.sin_addr));
		if (verify) {
			if (mode == 1 || mode == 2) {
				printf("\t  Checking IP uniqueness... ");
			}
			if (seenIPs.find(std::string(IP_str)) != seenIPs.end()) {
				ipsCount++;
				if (mode == 1 || mode == 2) {
					printf("failed.\n");
				}
				return 1;
			}
			passedIPUniqueness++;
			if (mode == 1 || mode == 2) {
				printf("passed.\n");
				printf("\t  Doing DNS... ");
				printf("done in %d ms, found %s\n", 0, inet_ntoa(server.sin_addr));
			}
		}
	}

	return 0;
}


int Socket::connectToServer(SOCKET sock, hostent *remote, sockaddr_in &server, char *host, char *port, bool isRobots, int mode) {

	// setup the port # and protocol type
	server.sin_family = AF_INET;
	server.sin_port = (port == nullptr) ? htons(80) : htons(atoi(port));		// host-to-network flips the byte order

	clock_t connectStart = clock();
	// connect to the server on port
	if (connect(sock, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		if (isRobots) {
			if (mode == 1 || mode == 2) {
				printf("\t  Connecting on robots... failed with %d\n", WSAGetLastError());
			}
			return 1;
		}
		else {
			if (mode == 1 || mode == 2) {
				printf("\t* Connecting on page... failed with %d\n", WSAGetLastError());
			}
			return 1;
		}
	}

	clock_t connectDuration = clock() - connectStart;
	if (mode == 1 || mode == 2) {
		if (isRobots) {
			printf("\t  Connecting on robots... done in %d ms\n", connectDuration);
		}
		else {
			printf("\t* Connecting on page... done in %d ms\n", connectDuration);
		}
	}

	return 0;
}

// Mostly from pseudocode from hw pdf
bool Socket::Read(bool isRobots, int mode) {

	// set timeout for 10 sec
	timeval timeVal;
	timeVal.tv_sec = 10;		// Time interval in seconds
	timeVal.tv_usec = 0;		// Time interval in microseconds

	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(sock, &fd);

	// if the buffer is > 32KB resize to initial size
	if (curPos > (1024 * 32)) {
		char* newBuf = (char*)realloc(buf, INITIAL_BUF_SIZE);
		// Check if buffer might have been turned to nullptr
		if (newBuf == nullptr) {
			if (mode == 1 || mode == 2) {
				printf("Error resizing buffer during read\n");
			}
			free(buf);
			return false;
		}
		buf = newBuf;
	}

	clock_t readStart = clock();
	curPos = 0;
	while (true) {

		// wait to see if socket has any data see MSDN
		int ret;
		if ((ret = select(0, &fd, NULL, NULL, &timeVal)) > 0) {
			// new data available; now read the next segment
            int bytes = recv(sock, buf + curPos, (allocatedSize - curPos), 0);
			if (bytes == SOCKET_ERROR) {
				if (mode == 1 || mode == 2) {
					printf("\t  Loading... failed with %d on recv\n", WSAGetLastError());
				}
				return false;
			}
			else if (bytes == 0) {
				// NULL-terminate buffer
				buf[curPos] = '\0';

				if ( (strncmp(buf, "HTTP/", 5) != 0) && !isRobots) {
					if (mode == 1 || mode == 2) {
						printf("\t  Loading... failed with non-HTTP header (does not begin with HTTP/)\n");
					}
					return false;
				}
				clock_t readDuration = clock() - readStart;
				if (mode == 1 || mode == 2) {
					if (isRobots) {
						printf("\t  Loading... done in %d ms with %d bytes\n", readDuration, curPos);
					}
					else {
						printf("\t  Loading page... done in %d ms with %d bytes\n", readDuration, curPos);
					}
				}
				return true; // normal completion
			}

			// Move position to next chunk to read
			curPos += bytes; // adjust where the next recv goes
			if (allocatedSize - curPos < READ_THRESHOLD) {
				// resize buffer; you can use realloc(), HeapReAlloc(), or
				// memcpy the buffer into a bigger array
				allocatedSize *= 2;
				// For null char
				allocatedSize++;
				char* newBuf = (char*)realloc(buf, allocatedSize);
				// Check if buffer might have been turned to nullptr
				if (newBuf == nullptr) {
					if (mode == 1 || mode == 2) {
						printf("Error resizing buffer during read\n");
					}
					free(buf);
					return false;
				}
				buf = newBuf;
			}

			clock_t elapsedTime = clock() - readStart;
			double timeCheck = (double)elapsedTime / CLOCKS_PER_SEC;
			if (timeCheck > 10) {
				if (mode == 1 || mode == 2) {
					printf("\t  Loading... failed with slow download\n");
				}
				return false;
			}

			if ( (curPos > MAX_PAGE_SIZE) && !isRobots) {
				if (mode == 1 || mode == 2) {
					printf("\t  Loading page... failed with exceeding max");
				}
				return false;
			}

			if ( (curPos > MAX_ROBOTS_PAGE_SIZE) && isRobots) {
				if (mode == 1 || mode == 2) {
					printf("\t  Loading page... failed with exceeding max");
				}
				return false;
			}
		}
		else if (ret == 0) {
			if (mode == 1 || mode == 2) {
				printf("\t  Loading... failed with timeout\n");
			}
			return false;
		}
		else {
			if (mode == 1 || mode == 2) {
				printf("Loading... failed with select error %d\n", WSAGetLastError());
			}
			return false;
		}
	}
	return true;
}

int Socket::sendHTTPRequest(char *host, char *path, char *query, bool isRobots, int mode) {
	char request[REQUEST_BUF_SIZE];
	if (isRobots) {
		sprintf_s(request, "HEAD %s%s HTTP/1.0\r\nUser-agent: myTAMUCrawler/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, query, host);
	}
	else {
		sprintf_s(request, "GET %s%s HTTP/1.0\r\nUser-agent: myTAMUCrawler/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, query, host);    // sprintf is deprecated, use secure
	}
	//printf("GET %s%s HTTP/1.0\r\nUser-agent: myTAMUCrawler/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", path, query, host);
	if (send(sock, request, strlen(request), 0) == SOCKET_ERROR) {
		if (mode == 1 || mode == 2) {
			printf("Send request failed with %d\n", WSAGetLastError());
		}
		return 1;
	}
}

int Socket::parseHTTPResponse(char *host, bool isRobots, int mode) {

	// Check response header
	char* responseHeader = strstr(buf, "\r\n\r\n");
	if (responseHeader != nullptr) {
		*responseHeader = '\0';
	}
	else {
		if (mode == 1 || mode == 2) {
			printf("No response header found\n");
		}
		return 1;
	}

	// Get status code
	int statusCode = atoi(buf + 9);

	if (mode == 1 || mode == 2) {
		printf("\t  Verifying header... ");

		printf("Status code %d\n", statusCode);
	}
	// Request is not 2xx skip parsing HTML
	if ((statusCode < 200 || statusCode >= 300) && !isRobots) {
		if (mode == 1 || mode == 2) {
			printf("----------------------------------------\n");
			printf("%s", buf);
		}
		return 0;
	}
	else if (isRobots && (statusCode < 400 || statusCode >= 500)) {
		// If status code is not 4xx skip parsing page
		// Other prevent further contact
		return 1;
	}
	else if (!isRobots) {
		if (mode == 1 || mode == 2) {
			printf("\t+ Parsing page... ");
		}
		clock_t parseStart = clock();

		// From HTMLParserTest.cpp example
		HTMLParserBase* parser = new HTMLParserBase;

		// Add scheme to host
		char *scheme = (char*)"http://";
		int baseLen = (strlen(scheme) + strlen(host) + 1) * sizeof(char);
		char* baseURL = (char*)malloc(baseLen);
		if (baseURL == nullptr) {
			if (mode == 1 || mode == 2) {
				printf("Error allocating memory for baseURL\n");
			}
			return 1;
		}
		strcpy_s(baseURL, baseLen, scheme);
		strcat_s(baseURL, baseLen, host);
		//printf("Base URL: %s\n", baseURL);

		// From HTMLParserTest.cpp example
		int nLinks;
		char* linkBuffer = parser->Parse(buf, curPos, baseURL, (int)strlen(baseURL), &nLinks);
		free(baseURL);

		// check for errors indicated by negative values
		if (nLinks < 0) {
			if (mode == 1 || mode == 2) {
				printf("Error in nLinks: %d\n", nLinks);
			}
			nLinks = 0;
			return 1;
		}

		// print each URL; these are NULL-separated C strings
		for (int i = 0; i < nLinks; i++)
		{
			//printf("%s\n", linkBuffer);
			linkBuffer += strlen(linkBuffer) + 1;
		}

		delete parser;		// this internally deletes linkBuffer

		clock_t parseDuration = clock() - parseStart;
		if (mode == 1 || mode == 2) {
			printf("done in %d ms with %d links\n", parseDuration, nLinks);
			printf("----------------------------------------\n");
			printf("%s", buf);
		}
	}

	return 0;
}

int Socket::_parseHTTPResponse(char *host, bool isRobots) {

	// Check response header
	char* responseHeader = strstr(buf, "\r\n\r\n");
	if (responseHeader != nullptr) {
		*responseHeader = '\0';
	}
	else {
		printf("No response header found\n");
		return 1;
	}

	// Get status code
	int statusCode = atoi(buf + 9);

	printf("\t  Verifying header... ");

	printf("Status code %d\n", statusCode);
	// Request is not 2xx skip parsing HTML
	if ((statusCode <= 200 || statusCode >= 300) && !isRobots) {
		return 0;
	}
	else if (isRobots && (statusCode < 400 || statusCode >= 500)) {
		// If status code is not 4xx skip parsing page
		// Other prevent further contact
		return 1;
	}
	else if (!isRobots) {
		printf("\t+ Parsing page... ");
		clock_t parseStart = clock();

		// From HTMLParserTest.cpp example
		HTMLParserBase* parser = new HTMLParserBase;

		// Add scheme to host
		char *scheme = (char*)"http://";
		int baseLen = (strlen(scheme) + strlen(host) + 1) * sizeof(char);
		char* baseURL = (char*)malloc(baseLen);
		if (baseURL == nullptr) {
			printf("Error allocating memory for baseURL\n");
			return 1;
		}
		strcpy_s(baseURL, baseLen, scheme);
		strcat_s(baseURL, baseLen, host);
		//printf("Base URL: %s\n", baseURL);

		// From HTMLParserTest.cpp example
		int nLinks;
		char* linkBuffer = parser->Parse(buf, curPos, baseURL, (int)strlen(baseURL), &nLinks);
		free(baseURL);

		// check for errors indicated by negative values
		if (nLinks < 0) {
			printf("Error in nLinks: %d\n", nLinks);
			nLinks = 0;
			return 1;
		}

		// print each URL; these are NULL-separated C strings
		for (int i = 0; i < nLinks; i++)
		{
			//printf("%s\n", linkBuffer);
			linkBuffer += strlen(linkBuffer) + 1;
		}

		delete parser;		// this internally deletes linkBuffer

		clock_t parseDuration = clock() - parseStart;
		printf("done in %d ms with %d links\n", parseDuration, nLinks);
	}

	return 0;
}
