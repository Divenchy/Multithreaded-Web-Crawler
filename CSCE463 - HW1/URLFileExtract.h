#pragma once
#include "pch.h"

// Implementation of reading URLs from a file
// Uses input file of a specific format (1 url per line) to extract URLs into
// an array
// @param filename: The name of the file to read
int URLFileExtract(char* filename, char** &urls, int &count) {
	// Read file into buffer as show in HTMLParserTest.cpp 
	// Cause hw pdf said this is the fastest

	// A for ANSI
	HANDLE urlFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (urlFile == INVALID_HANDLE_VALUE) {
		// Get useful error info
		DWORD error = GetLastError();
		// 2 means not found
		printf("Error in creating file: %d\nError 2: file means not found\n", error);
		return 1;
	}

	// set file size
	LARGE_INTEGER fSize;
	BOOL bRet = GetFileSizeEx(urlFile, &fSize);
	if (bRet == 0) {
		printf("Error in obtaining file size. \n");
		return 1;
	}

	int bufferFileSize = (DWORD)fSize.QuadPart;
	DWORD bytesRead;
	char* fileBuffered = (char*)malloc(bufferFileSize * sizeof(char));
	if (fileBuffered == nullptr) {
		printf("Error allocating memory for file buffer\n");
		return 1;
	}
	bRet = ReadFile(urlFile, fileBuffered, bufferFileSize, &bytesRead, NULL);
	if (!bRet) {
		printf("Error reading file: %d\n", GetLastError());
		return 1;
	}

	fileBuffered[bytesRead] = '\0';	// Null terminate string

	// Done with file
	CloseHandle(urlFile);

	// Determine num of lines in file to set size of urls array
	// prob not good for long files aka reads the file again later
	//int lines = 0;
	//for (int i = 0; i < bufferFileSize; i++) {
	//	if (*(fileBuffered+i) == '\n') {
	//		lines++;
	//	}
	//}

	//printf("Num of lines: %d\n", lines);

	// Create c-string array
	urls = (char**)malloc(INITIAL_URL_ARRAY_SIZE * sizeof(char*));
	if (urls == nullptr) {
		printf("Error allocating memory for url array\n");
		return 1;
	}


	// Testing strtok, handy function that tokenizes strings through use of delimeters
	// Returns token, and if no token then returns NULL, but prob use nullptr since in c++ whatever version
	//char* token = strtok_s(fileBuffered, "\n", &fileBuffered);
	//printf("Token: %s", token);

	// Parse fileBuffered into urls array
	char* context = nullptr;
	char* token = strtok_s(fileBuffered, "\n", &context);  // using secure version to make windows happy
	int urlsSize = INITIAL_URL_ARRAY_SIZE;
	while (token != nullptr) {

		// Resize array when needed
		if (count == urlsSize) {
			urlsSize *= 2;
			char** newUrls = (char**)realloc(urls, urlsSize * sizeof(char*));
			if (newUrls == nullptr) {
				printf("Error reallocating memory for url array\n");
				free(urls);
				return 1;
			}
			urls = newUrls;
		}

		// Add token to array using handy function strdup to duplicate strings
		// Mindful of strdup, allocates memory using malloc
		urls[count] = _strdup(token);	
		if (urls[count] == nullptr) {
			printf("Error allocating memory for url\n");
			// probably free all elems in array as well
			for (int i = 0; i < count; i++) {
				free(urls[i]);
			}
			free(urls);
			return 1;
		}
		count++;

		// Get next token, passing NULL tells it to continue where it left off
		token = strtok_s(NULL, "\n", &context);
	}


	// See resulting array
	//for (int i = 0; i < count; i++) {
	//	printf("URL: %s\n", urls[i]);
	//}

	printf("Opened URL-input.txt with size %d\n", bytesRead);

	return 0;	// Clean exit
}

