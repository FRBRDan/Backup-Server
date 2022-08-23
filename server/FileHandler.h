/*
FileHandler.h
*/

#pragma once
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

class FileHandler {

public:
	bool openFile(const std::string&, std::fstream&, bool);
	bool closeFile(std::fstream&);
	bool readFileIntoPayload(std::fstream&, char*, uint32_t);
	bool writeToFile(std::fstream&, const char*, uint32_t);
	bool deleteFile(const std::string&);
	
	bool updateFileList(const std::string&, std::string&);
	bool isExistent(const std::string&);
	uint32_t getFileSize(const std::string&);

};