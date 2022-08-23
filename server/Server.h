#pragma once
#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string>
#include "FileHandler.h"
#include "Log.h"

#define PORT 80
#define PACKET_SIZE 1024
#define CURR_VERSION 1
#define REQUEST_OFFSET 12
#define RESPONSE_OFFSET 9

class Server {
	friend class Log;
#pragma pack(push, 1)
	struct Request {
		union URequestHeader {
			struct {
				uint32_t user_id;
				uint8_t version;
				uint8_t op;
				uint16_t name_len;
			} SRequestHeader;
			char buffer[sizeof(SRequestHeader)];
		} URequestHeader;
		char* filename;
		uint32_t size;
		char* payload;
	} _request;
	
	struct Response {
		union UResponseHeader {
			struct {
				uint8_t version;
				uint16_t status;
				uint16_t name_len;
			} SResponseHeader;
			char buffer[sizeof(SResponseHeader)];
		} UResponseHeader;
		char* filename;
		uint32_t size;
		char* payload;
	} _response;
/*
Server.h
*/

#pragma pack(pop)

	enum Op {UPLOAD_FILE = 100, DOWNLOAD_FILE = 200, DELETE_FILE = 201, REQUEST_ALL_FILES = 202};
	enum Status {FILE_SUCCESS_DOWNLOAD = 210, FILES_LIST_SENT = 211, FILE_SUCCESS_DL_DEL = 212,	FILE_NOT_FOUND = 1001, NO_FILES_FOR_USER = 1002, GENERAL_ERROR = 1003};

private:
	void unpackRequest(char*);
	void packResponse(char*);
	bool uploadFile(const SOCKET&, char*);
	bool downloadFile(const SOCKET&, char*);
	bool allFilesRequest(const SOCKET&, char*);
	bool deleteFile(const SOCKET&, char*);
	void unknownRequest(const SOCKET&, char*);

	uint32_t requestOffset() const;
	uint32_t responseOffset() const;
	void copyHeader();
	void sendAdditionalPayload(const SOCKET&, char*, const char*, uint32_t) const;
	
public:
	Server();
	~Server();
	void handleRequest(const SOCKET&);	
};