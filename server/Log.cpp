/* Log.cpp
Deals with logging the operations that happen on the server */

#include "Log.h"

/* Logs a message upon successful upload */
void Log::successUpload(const Server* sHandler) const
{	
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::string filename = sHandler->_request.filename;
	std::cout << "User " << user_id << " successfully uploaded file: " << filename << "." << std::endl;
}

/* Logs a message upon successful download */
void Log::successDownload(const Server* sHandler) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::string filename = sHandler->_request.filename;
	std::cout << "User " << user_id << " successfully downloaded file: " << filename << "." << std::endl;
}

/* Logs a message upon successful delete */
void Log::successDelete(const Server* sHandler) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::string filename = sHandler->_request.filename;
	std::cout << "User " << user_id << " successfully DELETED file: " << filename << " from the server." << std::endl;
}

/* Logs a message upon successful request to view all files */
void Log::successFilesList(const Server* sHandler) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::cout << "Files list of user " << user_id << " were successfully submitted." << std::endl;
}

/* Logs an error message when a requested file is not found */
void Log::errorFileNotFound(const Server* sHandler) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::cout << "Error: User " << user_id << " requested a non-existent file. " << std::endl;
}

void Log::errorNoFolder(const Server* sHandler) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::cout << "Error: Folder for user " << user_id << " was not found." << std::endl;

}

void Log::errorsFound(const Server* sHandler, const std::string& operation) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::cout << "Error(s) found for user " << user_id << " during " << operation << "." << std::endl;
}

void Log::generalError(const Server* sHandler) const
{
	uint32_t user_id = sHandler->_request.URequestHeader.SRequestHeader.user_id;
	std::cout << "General Error for " << user_id << " , (sent invalid Op)." << std::endl;
}
