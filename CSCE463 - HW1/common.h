// Leonardo Frias CSCE463-500 Spring 2025

#pragma once
#include "pch.h"

#define REQUEST_BUF_SIZE 512
#define READ_THRESHOLD 2048			// Resize buffer when less that 2KB is left
#define MAX_PAGE_SIZE 2097152	// 2MB Page Size Limit
#define MAX_ROBOTS_PAGE_SIZE 16384	// 16KB Robots.txt Size Limit

extern int INITIAL_BUF_SIZE;
extern int INITIAL_URL_ARRAY_SIZE;
extern int hostsCount;
extern int ipsCount;

extern std::set<std::string> seenHosts;  // Use custom comparator for char* in set
extern std::set<std::string> seenIPs;    // Use custom comparator for IPs
