// Leonardo Frias CSCE463-500 Spring 2025

#include "pch.h"
#include "Socket.h"
#include "Crawler.h"
#include "URLFileExtract.h"


// Threading info: https://learn.microsoft.com/en-us/windows/win32/sync/using-event-objects

int main(int argc, char** argv)
{

	// WSAStartup
	WSADATA wsaData;

	//Initialize WinSock; once per program run
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		// Called in deconstructor WSACleanup();
		return 1; 
	}

	// Wrong usage
	if (argc < 2 || argc > 3) {
		printf("Usage of program: hw1.exe <url>  or   hw1.exe <num of threads> <txt file>");
		return 1;
	}

	// Num of threads specified == 0
	int numThreads = atoi(argv[1]);
	if (numThreads == 0) {
		printf("Invalid number of threads. Did you put a number?\n");
		return 1;
	}

	// Make Crawler object (threads)
	Crawler *c = new Crawler(argv[2], numThreads);

	// Extract urls from file, then create url queue
	// use nullptr instead of NULL since also using C++
	char **urls = nullptr;
	int count = 0;
	URLFileExtract(argv[2], urls, count);

	// Add urls to queue
	for (int i = 0; i < count; i++) {
		c->queueURL(urls[i]);
	}

	//// Print to check if queue is correct
	//c->printURLQueue();
	//c->printURLQueueSize();

	clock_t startTime = clock();
	// Start running threads also waits for all threads to finished (through monitoring thread)
	c->initThreads(numThreads);

	// Wait for all threads to finish
	c->waitForThreads();

	//c->printURLQueue();
	
	//c->printURLQueueSize();
	// Threads have finished print stats
	c->printSummary(startTime);
	//c->printSeenHostsAndIps();

	delete c;

	return 0;
}
