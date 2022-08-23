/*
Log.h 
*/
#pragma once

#include "Server.h"
#include <chrono>
#include <ctime>

class Server;

class Log {	
public:
	void successUpload(const Server*) const;
	void successDownload(const Server*) const;
	void successDelete(const Server*) const;
	void successFilesList(const Server*) const;
	
	void errorFileNotFound(const Server*) const;
	void errorNoFolder(const Server*) const;
	void errorsFound(const Server*, const std::string&) const;
	void generalError(const Server*) const;
};