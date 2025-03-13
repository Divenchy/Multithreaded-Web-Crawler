#pragma once
#include "pch.h"
#include "Socket.h"

// Probably don't need semaphores for this part
class Crawler {
private:
	struct sharedData {
		std::set<std::string> seenHosts;
		std::set<std::string> seenIPs;
		std::queue<char *> urlQueue;// Queue for pending urls here
		int E;		// Num of extracted urls
		int H;		// Num of unique hosts
		int D;		// Num of successful DNS lookups
		int I;		// Num of urls that passed IP uniqueness
		int R;		// Num of urls that passed robots check
		int C;		// Num of urls successfully crawled urls
		int L;		// Total links found
		int threadsFinished;

		// Struct constructor
		sharedData() {
			// Init seenHosts/IPs to empty sets
			seenHosts = std::set<std::string>();
			seenIPs = std::set<std::string>();
			urlQueue = std::queue<char *>();
			E = 0;
			H = 0;
			D = 0;
			I = 0;
			R = 0;
			C = 0;
			L = 0;
			threadsFinished = 0;
		};
	};
	// URL struct to handle url data easier
	struct ParsedURL {
		char *url_fragment = nullptr;
		char *url_query = nullptr;
		char *url_path = nullptr;
		char *url_port = nullptr;
		char *url_host = nullptr;

		bool hasPort = false;
	};

	// Handles for Threads
	HANDLE mutex;
	HANDLE finished;
	HANDLE eventQuit;

	// Array for sockets
	Socket *sockArr;	// might change to a vector or remove completely
	sharedData *shared;

	// Threads
	std::vector<HANDLE> threads;
	int numThreads;

public:
	/// Constructor
	Crawler(char* filename, int nThreads);
	// Destructor
	~Crawler();

	// Methods/Helpers 
	int URLParser(ParsedURL &parsedURL, char* url, int mode);
	void printParsedURL(ParsedURL &parsedURL);

	/// Thread Initialization
	void initThreads(int nThreads);

	/// Thread routines
	static DWORD WINAPI connectToUrl(LPVOID param);
	static DWORD WINAPI test(LPVOID lpParam);
	static DWORD WINAPI monitorStatus(LPVOID lpParam);

	/// Thread Safety
	void lock();
	void unlock();
	void signalThreadFinished();
	void waitForThreads();

	// Getters and Setters
	void queueURL(char *url);
	void printURLQueue();
	void printSummary(clock_t startTime);
	void printSeenHostsAndIps();
	void printURLQueueSize();
};

