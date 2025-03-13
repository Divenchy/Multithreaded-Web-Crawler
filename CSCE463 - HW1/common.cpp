// Leonardo Frias CSCE463-500 Spring 2025

#include "pch.h"
#include "common.h"

// Start with 8KB buffer
int INITIAL_BUF_SIZE = 8192;
int INITIAL_URL_ARRAY_SIZE = 100;
int hostsCount = 0;
int ipsCount = 0;

std::set<std::string> seenHosts;  // Use custom comparator for char* in set
std::set<std::string> seenIPs;    // Use custom comparator for IPs
