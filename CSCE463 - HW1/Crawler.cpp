#include "pch.h"
#include "Crawler.h"

///// signalThreadFinished() SHOULD ONLY BE USED ONCE INSIDE ROUTINE

// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-threads
// Using ReleaseSemaphore to increment the semaphore count so that semaphore can be init to 0 blocking it during initialization
Crawler::Crawler(char *filename, int nThreads) : sockArr(nullptr), shared(new sharedData()), numThreads(nThreads){
	// From sample
	// create a mutex for accessing critical sections (including printf); initial state = not locked
	mutex = CreateMutex(NULL, 0, NULL);
	// create a semaphore that counts the number of active threads; initial value = 0, max = 2
	finished = CreateSemaphore(NULL, 0, nThreads, NULL);
	// create a quit event; manual reset, initial state = not signaled
	eventQuit = CreateEvent(NULL, true, false, NULL);
}

Crawler::~Crawler() {
	waitForThreads();

	// Close thread handles
	for (HANDLE thread : threads) {
		CloseHandle(thread);
	}

	// Close handles
	CloseHandle(mutex);
	CloseHandle(finished);
	CloseHandle(eventQuit);
	delete shared;
}

///// THREADSS

void Crawler::initThreads(int nThreads) {
	// Initialize threads (aka CreateThreads)

	// Monitoring thread
	DWORD monitorThreadID;
	HANDLE monitorThread = CreateThread(NULL, 0, monitorStatus, this, 0, &monitorThreadID);
	if (monitorThread == NULL) {
		printf("Monitor thread creation failed\n");
		return;
	}
	threads.push_back(monitorThread);

	// Worker threads that connect to urls
	for (int i = 0; i < nThreads; i++) {
		// Create a thread
		DWORD threadID;
		HANDLE thread = CreateThread(NULL, 0, connectToUrl, this, 0, &threadID);
		if (thread == NULL) {
			printf("Thread creation failed\n");
			return;
		}
		threads.push_back(thread);
	}
}

// Thread routines
DWORD WINAPI Crawler::test(LPVOID lpParam) {
	printf("Thread started\n");
	return 0;
}

// Status monitor
////// Mutexes slow alot
DWORD WINAPI Crawler::monitorStatus(LPVOID lpParam) {
	Crawler *crawl = static_cast<Crawler *>(lpParam);
	sharedData *shared = crawl->shared;
	clock_t monitoringStart = clock();

	// Keep printing stats every 2 seconds
	while (WaitForSingleObject(crawl->eventQuit, 2000) == WAIT_TIMEOUT) {
		//Sleep(2000);
		// Attempt to enter semaphore gate
		// WaitForSingleObject takes the semaphore handle and set to 0 second timeout
		//crawl->lock();
		//if (shared->threadsFinished == crawl->numThreads) {
		//if (WaitForSingleObject(crawl->eventQuit, 2000) == WAIT_TIMEOUT) {
		//	// Quit handle for monitor thread
		//	SetEvent(crawl->eventQuit);
		//	return 0;
		//}
		//crawl->unlock();

		// Get number of active threads
		int numActive = crawl->numThreads - shared->threadsFinished;
		LONG urlQueueSize = (LONG)shared->urlQueue.size();
		LONG poppedURLs = shared->E;
		LONG uniqueHosts = shared->H;
		LONG successfulDNS = shared->D;
		LONG successfulIPs = shared->I;
		LONG successfulRobots = shared->R;
		LONG crawledPages = shared->C;
		LONG linksfound = shared->seenHosts.size();
		clock_t uptime = clock() - monitoringStart;
		// Format uptime to seconds
		// Print stats
		printf("[%3d] %6d Q %6d E %7d H %6d D %6d I %3ld R %5d C %5d L %4d\n", (int)(uptime / CLOCKS_PER_SEC), numActive, urlQueueSize, poppedURLs, uniqueHosts, successfulDNS, successfulIPs, successfulRobots, crawledPages, linksfound);
	}

	return 1;
}

DWORD WINAPI Crawler::connectToUrl(LPVOID lpParam) {
	// Way to access the shared data
	Crawler *crawl = static_cast<Crawler *>(lpParam);
	sharedData *shared = crawl->shared;

	if (shared == nullptr) {
		printf("Error getting crawl class shared data!");
		return 1;
	}

	//crawl->lock();
	//printf("This is thread %d\n", GetCurrentThreadId());
	//crawl->unlock();

	// Get URL from queue, if queue still has urls keep going
	while (true) {
		crawl->lock();
		if (shared->urlQueue.empty()) {
			crawl->unlock();
			//printf("I am done with my work\n");
			//crawl->signalThreadFinished();
			return 0;
		}

		// Make copy of url
		char *url = _strdup(shared->urlQueue.front());
		shared->urlQueue.pop();
		shared->E++;
		crawl->unlock();

		// DO robots
		char* IP_str = nullptr;
		bool lookupDone = false;
		int mode = 3;
		Socket *s = new Socket();
		ParsedURL parsedURL;
		crawl->lock();
		if (crawl->URLParser(parsedURL, url, mode) == 1) {
			crawl->unlock();
			free(url);
			continue;
		}

		//printf("URL: %s\n", url);
		crawl->unlock();

		// Verify url for robots
		crawl->lock();
		if (s->DNSLookup(s->sock, s->remote, s->server, parsedURL.url_host, parsedURL.url_port, lookupDone, IP_str, true, mode, shared->D, shared->I) == 1) {
			crawl->unlock();		// No increment on D but still need to unlock
			continue;
		}
		crawl->unlock();


		// Passed host and ip uniqueness if got to this point
		crawl->lock();
		shared->H++;
		if (lookupDone) {
			shared->seenIPs.insert(std::string(IP_str));
			shared->seenHosts.insert(std::string(parsedURL.url_host));
		}
		crawl->unlock();

		// Attempt to connect
		if (s->connectToServer(s->sock, s->remote, s->server, parsedURL.url_host, parsedURL.url_port, true, mode) == 1) {
			continue;
		}

		char* path = (char*)"/robots.txt";
		char* query = (char*)"";
		if (s->sendHTTPRequest(parsedURL.url_host, path, query, true, mode) == 1) {
			continue;
		}

		// Read response
		if (s->Read(true, mode) == false) {
			continue;
		}

		// Parse HTML
		// If robots.txt is not found, skip download and add to seen
		if (s->parseHTTPResponse(parsedURL.url_host, true, mode) == 1) {
			closesocket(s->sock);
			continue;
		}

		closesocket(s->sock);

		// If got this far, download page
		crawl->lock();
		shared->R++;
		crawl->unlock();

		Socket *s2 = new Socket();

		// Ip and host uniqueness already checked
		crawl->lock();
		if (s2->DNSLookup(s2->sock, s2->remote, s2->server, parsedURL.url_host, parsedURL.url_port, lookupDone, IP_str, false, mode, shared->D, shared->I) == 1) {
			crawl->unlock();
			continue;
		}
		crawl->unlock();

		// Attempt to connect
		if (s2->connectToServer(s2->sock, s2->remote, s2->server, parsedURL.url_host, parsedURL.url_port, false, mode) == 1) {
			continue;
		}

		if (s2->sendHTTPRequest(parsedURL.url_host, parsedURL.url_path, parsedURL.url_query, false, mode) == 1) {
			continue;
		}

		// Read response
		if (s2->Read(false, mode) == false) {
			continue;
		}

		// Parse HTML
		if (s2->parseHTTPResponse(parsedURL.url_host, false, mode) == 1) {
			continue;
		}

		// If got here then page was successfully crawled
		crawl->lock();
		shared->C++;
		crawl->unlock();
		closesocket(s2->sock);
		free(url);
	}

	return 0;
}

////// Setters
void Crawler::queueURL(char *url) {
	shared->urlQueue.push(url);
}


///// Getters
void Crawler::printURLQueue() {

	while (!shared->urlQueue.empty()) {
		printf("URL: %s\n", shared->urlQueue.front());
		shared->urlQueue.pop();
	}
}

void Crawler::printURLQueueSize() {
	printf("URL Queue size: %d\n", (int)shared->urlQueue.size());
}

void Crawler::printSummary(clock_t startTime) {
	clock_t runtime = clock();
	double elapsedTime = (double)(runtime - startTime) / CLOCKS_PER_SEC;
	printf("Extracted %d URLs @ %.0f/s\n", shared->E, shared->E / elapsedTime);
	printf("Looked up %d DNS names @ %.0f/s\n", shared->D, shared->D / elapsedTime);
	printf("Attempted %d site robots @ %.0f/s\n", shared->R, shared->R / elapsedTime);
	// Might need to change MB calculation
	printf("Crawled %d pages @ %.0f/s (%.2f MB)\n", shared->C, shared->C / elapsedTime, (double)shared->C * 0.1);
	printf("Parsed %d links @ %.0f/s\n", shared->L, shared->L / elapsedTime);
	// Need to fix this
	printf("HTTP codes: 2xx = %d, 3xx = %d, 4xx = %d, 5xx = %d, other = 0\n", 0, 0, 0, 0); // Adjust HTTP codes as needed
}

void Crawler::printSeenHostsAndIps() {
	printf("SEEN HOSTS:\n");
	for (const std::string &host : shared->seenHosts) {
		printf("%s\n", host.c_str());
	}

	printf("SEEN IPS:\n");
	for (const std::string &ip : shared->seenIPs) {
		printf("%s\n", ip.c_str());
	}
}

///// Methods to make more sense of their purpose
void Crawler::lock() {
	WaitForSingleObject(mutex, INFINITE);
}

void Crawler::unlock() {
	ReleaseMutex(mutex);
}

// Increment semaphore towards max count 
void Crawler::signalThreadFinished() {
	lock();
	shared->threadsFinished++;
	unlock();
	ReleaseSemaphore(finished, 1, NULL);
}

// Wait for all threads to finish
void Crawler::waitForThreads() {
	// Monitor thread is also added to threads vector
	// Monitor thread is at index 0 so don't wait for it
	for (int i = 1; i < threads.size(); i++) {
		WaitForSingleObject(threads[i], INFINITE);
	}
	SetEvent(eventQuit);
	WaitForSingleObject(threads[0], INFINITE);
}



// URLParser

void Crawler::printParsedURL(ParsedURL &parsedURL) {
	// From slides request is [/path][?query]
	printf("host %s, port %s\n", parsedURL.url_host, parsedURL.url_port);
};

void parseAndTruncateCrawler(char *parameterStr, char ch, char *&parameter, char *&result, char *url) {
	result = strchr(url, ch);
	if (result != nullptr) {
		parameter = (char*)malloc(strlen(result) * sizeof(char) + 1*sizeof(char));
		if (parameter == nullptr) {
			printf("Memory allocation failed for parameter. [URLParser.cpp]");
			return;
		}
		// Realized need delimeters for output and requests
		// Safe copy to parameter
		int check = strcpy_s(parameter, strlen(result) + 1, result);
		if (check != 0) {
			printf("Error has occured in copying sub string to parameter. [URLParser.cpp]");
			return;
		}

		//printf("%s: %s\n", parameterStr, parameter);
		//printf("RESULT: %s\n", result);       // prints out #something
		*result = '\0';						  // Truncate
		//printf("TRUNCATED STRING: %s\n", url);
	}
};

// From class 
// 1. Find fragment first and truncate the string
// 2. Find query and truncate the string
// 3. Find path and truncate the string
// 4. Find port and truncate the string

int Crawler::URLParser(ParsedURL &parsedURL, char *url, int mode) {

	// Have to instantiate to a val so they can be used :(
	char *url_result = nullptr;
	if (mode == 1 || mode == 2) {
		printf("\t  Parsing URL... ");
	}
	// Check for http:// scheme
	char* http = (char*)"http://";
	if (strncmp(url, http, strlen(http)) == 0) {
		// Is http:// move ahead
		url = url + 7;
	}
	else {
		if (mode == 1 || mode == 2) {
			printf("failed with invalid scheme");
		}
		return 1;
	}
	//printf("NEW: %s\n", url);

	// Parse
	parseAndTruncateCrawler((char*)"FRAGMENT", '#', parsedURL.url_fragment, url_result, url);
	parseAndTruncateCrawler((char*)"QUERY", '?', parsedURL.url_query, url_result, url);
	if (parsedURL.url_query == nullptr) {
		parsedURL.url_query = (char*)"";
	}
	parseAndTruncateCrawler((char*)"PATH", '/', parsedURL.url_path, url_result, url);
	if (parsedURL.url_path == nullptr) {
		// if no path is present, then set to root
		parsedURL.url_path = (char*)"/";
	}
	parseAndTruncateCrawler((char*)"PORT", ':', parsedURL.url_port, url_result, url);
	// Remove the : from the port, move pointer up
	if (parsedURL.url_port != nullptr) {
		// Check if ":" is followed by a number
		parsedURL.url_port = parsedURL.url_port + 1;
		if (atoi(parsedURL.url_port) == 0 || (!isdigit(*parsedURL.url_port)) ) {
			printf("failed with invalid port\n");
			return 1;	
		}
		parsedURL.hasPort = true;
	}
	else {
		parsedURL.url_port = (char*)"80";
	}

	// strcpy is deprecated so using strcpy_s (secure version)
	// Safe copy the remaining of the url string to url_host
	parsedURL.url_host = (char*) malloc(strlen(url)*sizeof(char));
	if (parsedURL.url_host == nullptr) {
		printf("Memory allocation failed for url_host. [URLParser.cpp]");
		return 1;
	}
	int check = strcpy_s(parsedURL.url_host, strlen(url) + 1, url);
	if (check != 0) {
		printf("Error has occured in copying string. [URLParser.cpp]");
		return 1;
	}
	
	if (mode == 1 || mode == 2) {
		printParsedURL(parsedURL);
	}
	//printf("HOST: %s\n", parsedURL.url_host);
};
