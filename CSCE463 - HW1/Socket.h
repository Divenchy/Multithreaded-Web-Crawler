// Leonardo Frias CSCE463-500 Spring 2025
#pragma once
#include "pch.h"

class Socket
{
public:
	SOCKET sock;

	// From sample code ==> [1]
	// Structure used in DNS lookups [1]
	struct hostent* remote;

	// Connect to the server   [1]
	struct sockaddr_in server;

	// From hw pdf
	char* buf;				// current buffer
	int allocatedSize;		// bytes allocated for buf
	int curPos;				// current position in the buffer	

	Socket();
	~Socket();
	int connectToServer(SOCKET sock, hostent *remote, sockaddr_in &server, char *host, char *port, bool isRobots, int mode);
	int sendHTTPRequest(char *host, char *path, char *query, bool isRobots, int mode);
	int DNSLookup(SOCKET sock, hostent *remote, sockaddr_in &server, char* host, char* port, bool &lookupDone, char* &IP_str, bool verify, int mode, int &successLookups, int &passedIPUniqueness);
	bool Read(bool isRobots, int mode);
	int parseHTTPResponse(char *host, bool isRobots, int mode);
	int _parseHTTPResponse(char *host, bool isRobots);
};
