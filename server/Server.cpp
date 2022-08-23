/* Server.cpp
Handles the the server's back-end. */

#include "Server.h"
#include <direct.h>
#include <algorithm>

/* Unpacks the buffer received into the Request struct according to the given protocol. */
void Server::unpackRequest(char* clientmsg)
{
	memcpy(_request.URequestHeader.buffer, clientmsg, sizeof(_request.URequestHeader));
	char* ptr = clientmsg + sizeof(_request.URequestHeader);
	uint16_t name_len = _request.URequestHeader.SRequestHeader.name_len;
	_request.filename = new char[name_len + 1];
	memcpy(_request.filename, ptr, name_len);
	ptr += name_len;
	_request.filename[name_len] = 0;
	
	memcpy(&_request.size, ptr, sizeof(uint32_t));

	ptr += sizeof(uint32_t);

	unsigned int payloadSize = PACKET_SIZE - requestOffset();
	_request.payload = new char[payloadSize];
	memcpy(_request.payload, ptr, payloadSize);
}

/* Packs the Response struct into a buffer according to the given protocol. */
void Server::packResponse(char* buffer)
{
	memcpy(buffer, _response.UResponseHeader.buffer, sizeof(_response.UResponseHeader));
	char* ptr = buffer + sizeof(_response.UResponseHeader);
	uint16_t name_len = _response.UResponseHeader.SResponseHeader.name_len;
	if (_response.filename != nullptr) 
		memcpy(ptr, _response.filename, name_len);
	ptr += name_len;
	memcpy(ptr, &_response.size, sizeof(uint32_t));
	ptr += sizeof(uint32_t);
	if (_response.payload == nullptr)
		return;

	uint32_t currPayload = _response.size < PACKET_SIZE - responseOffset() ? _response.size : PACKET_SIZE - responseOffset();
	memcpy(ptr, _response.payload, currPayload);	
}

/* Deals with a request to upload a file. */
bool Server::uploadFile(const SOCKET& clientsocket, char* clientmsg)
{
	FileHandler fHandler;
	Log logger;
	char responseBuffer[PACKET_SIZE] = { 0 };
	std::fstream newFile;
	std::string fileDestination = "c:\\backupsvr\\" + std::to_string(_request.URequestHeader.SRequestHeader.user_id) + "\\" + _request.filename;
	uint32_t currSize = PACKET_SIZE - requestOffset() < _request.size ? PACKET_SIZE - requestOffset() : _request.size;

	if (fHandler.openFile(fileDestination, newFile, true)) {
		fHandler.writeToFile(newFile, _request.payload, currSize);
	}
	else {
		packResponse(responseBuffer);
		send(clientsocket, responseBuffer, PACKET_SIZE, 0);
		return false;
	}	

	uint32_t sizeLeft = _request.size - currSize;

	// If payload exceeds PACKET_SIZE, the server will continue to receive more data and append it to the file created.
	while (sizeLeft > 0) {		 
		recv(clientsocket, clientmsg, PACKET_SIZE, 0);
		currSize = sizeLeft < PACKET_SIZE ? sizeLeft : PACKET_SIZE;
		fHandler.writeToFile(newFile, clientmsg, currSize);
		sizeLeft -= currSize;
	}

	copyHeader();
	_response.UResponseHeader.SResponseHeader.status = FILE_SUCCESS_DL_DEL;
	packResponse(responseBuffer);
	send(clientsocket, responseBuffer, PACKET_SIZE, 0);	
	logger.successUpload(this);
	if (!fHandler.closeFile(newFile)) 
		return false;
	return true;
}

/* Deals with a request to download a file. */
bool Server::downloadFile(const SOCKET& clientsocket, char* buffer)
{
	FileHandler fHandler;
	Log logger;
	std::fstream requestedFile;
	std::string fileDestination = "c:\\backupsvr\\" + std::to_string(_request.URequestHeader.SRequestHeader.user_id) + "\\" + _request.filename;
	if (!fHandler.isExistent(fileDestination)) {
		copyHeader();
		_response.UResponseHeader.SResponseHeader.status = FILE_NOT_FOUND;
		logger.errorFileNotFound(this);
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
		return false;
	}
	_response.size = fHandler.getFileSize(fileDestination);
	_response.payload = new char[_response.size];
	uint32_t currPayload = _response.size < PACKET_SIZE - responseOffset() ? _response.size : PACKET_SIZE - responseOffset();

	if (fHandler.openFile(fileDestination, requestedFile, false)) {
		copyHeader();
		_response.UResponseHeader.SResponseHeader.status = FILE_SUCCESS_DOWNLOAD;
		fHandler.readFileIntoPayload(requestedFile, _response.payload, _response.size);
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
	}
	else {
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
		return false;
	}

	uint32_t sizeLeft = _response.size - currPayload;
	char* ptr = _response.payload + currPayload;

	sendAdditionalPayload(clientsocket, buffer, ptr, sizeLeft);
	
	ptr = nullptr;

	logger.successDownload(this);
	if (!fHandler.closeFile(requestedFile))
		return false;
	return true;
}

/* Deals with a request to receive a list of all files for user */
bool Server::allFilesRequest(const SOCKET& clientsocket, char* buffer)
{
	FileHandler fHandler;
	Log logger;
	std::string folderDestination = "c:\\backupsvr\\" + std::to_string(_request.URequestHeader.SRequestHeader.user_id);
	std::string listOfFiles = "";
	if (fHandler.updateFileList(folderDestination, listOfFiles)) {
		copyHeader();
		_response.UResponseHeader.SResponseHeader.status = FILES_LIST_SENT;
		_response.payload = new char[listOfFiles.length() + 1];
		strcpy_s(_response.payload, listOfFiles.length() + 1 ,listOfFiles.c_str());
		_response.size = listOfFiles.length() + 1;
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
	}
	else { 
		_response.UResponseHeader.SResponseHeader.status = NO_FILES_FOR_USER;
		logger.errorNoFolder(this);
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
		return false;
	}
	uint32_t currPayload = _response.size < PACKET_SIZE - responseOffset() ? _response.size : PACKET_SIZE - responseOffset();
	uint32_t sizeLeft = _response.size - currPayload;
	char* payloadPtr = _response.payload + currPayload;

	sendAdditionalPayload(clientsocket, buffer, payloadPtr, sizeLeft);
	
	payloadPtr = nullptr;

	logger.successFilesList(this);
	return true;
}

/* Deals with deletion if requested file. */
bool Server::deleteFile(const SOCKET& clientsocket, char* buffer)
{
	FileHandler fHandler;
	Log logger;
	std::fstream fileDelete;
	std::string fileDestination = "c:\\backupsvr\\" + std::to_string(_request.URequestHeader.SRequestHeader.user_id) + "\\" + _request.filename;
	if (!fHandler.isExistent(fileDestination)) { // Add log message
		_response.UResponseHeader.SResponseHeader.status = FILE_NOT_FOUND;
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
		return false;
	}

	if (!fHandler.deleteFile(fileDestination)) { // General Error
		packResponse(buffer);
		send(clientsocket, buffer, PACKET_SIZE, 0);
		return false;
	}

	copyHeader();
	_response.UResponseHeader.SResponseHeader.status = FILE_SUCCESS_DL_DEL;
	packResponse(buffer);
	send(clientsocket, buffer, PACKET_SIZE, 0);
	logger.successDelete(this);
	
	return true;
}

/* Packs the response as is with default GENERAL ERROR flag and sends it to user. */
void Server::unknownRequest(const SOCKET& clientsocket, char* buffer)
{
	Log logger;
	packResponse(buffer);
	send(clientsocket, buffer, PACKET_SIZE, 0);
	logger.generalError(this);
}

/* Returns offset for payload for a Request struct. */
uint32_t Server::requestOffset() const
{
	return REQUEST_OFFSET + _request.URequestHeader.SRequestHeader.name_len;
}

/* Returns offset for payload for a Response struct. */
uint32_t Server::responseOffset() const
{
	return RESPONSE_OFFSET + _request.URequestHeader.SRequestHeader.name_len; // Header is 9 bytes + name_length
}

/* Copies the header of Request struct into the Response struct */
void Server::copyHeader()
{
	uint16_t name_len = _request.URequestHeader.SRequestHeader.name_len;
	_response.UResponseHeader.SResponseHeader.name_len = name_len;
	_response.filename = new char[name_len + 1];
	std::copy_n(_request.filename, name_len, _response.filename);
}

/* The function keeps sending the payload until all of it was sent, in fragments, as PACKET_SIZE allows. */
void Server::sendAdditionalPayload(const SOCKET& clientsocket, char* buffer, const char* payloadPtr, uint32_t sizeLeft) const
{
	if (payloadPtr == nullptr)
		return;
	uint32_t currPayloadLength;
	while (sizeLeft > 0) {
		memset(buffer, 0, PACKET_SIZE);
		currPayloadLength = sizeLeft < PACKET_SIZE ? sizeLeft : PACKET_SIZE;
		memcpy(buffer, payloadPtr, currPayloadLength);
		send(clientsocket, buffer, PACKET_SIZE, 0);

		sizeLeft -= currPayloadLength;
		payloadPtr += currPayloadLength;
	}
}

/* Constructor & initialization of pointers to NULLPTR to avoid errors. */
Server::Server()
{
	_request.size = 0;	

	_request.filename = nullptr;
	_request.payload = nullptr;

	_response.filename = nullptr;
	_response.payload = nullptr;

	_response.UResponseHeader.SResponseHeader.version = CURR_VERSION;
	_response.UResponseHeader.SResponseHeader.status = GENERAL_ERROR; // Ultimately should be changed during runtime
	_response.UResponseHeader.SResponseHeader.name_len = 0;
	_response.size = 0;
}

/* If any memory was allocated, it will be deleted at this point. */
Server::~Server() 
{
	delete[] _request.filename;
	delete[] _request.payload;

	delete[] _response.filename;	
	delete[] _response.payload;
}

/* Deals with the request at hand and calls the necessary functions */
void Server::handleRequest(const SOCKET& clientsocket) {
	char clientmsg[PACKET_SIZE] = { 0 };
	recv(clientsocket, clientmsg, PACKET_SIZE, 0);
	Log logger;

	unpackRequest(clientmsg);

	switch (_request.URequestHeader.SRequestHeader.op) {
	case UPLOAD_FILE:
		if (!uploadFile(clientsocket, clientmsg))
			logger.errorsFound(this, "upload file");
		break;

	case DOWNLOAD_FILE:
		if (!downloadFile(clientsocket, clientmsg))
			logger.errorsFound(this, "download file");
		break;

	case DELETE_FILE:
		if (!deleteFile(clientsocket, clientmsg))
			logger.errorsFound(this, "file deletion");
		break;

	case REQUEST_ALL_FILES:
		if (!allFilesRequest(clientsocket, clientmsg))
			logger.errorsFound(this, "request for all files");
		break;

	default:
		unknownRequest(clientsocket, clientmsg);
		break;
	}	
}

