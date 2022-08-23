/* FileHandler.cpp
Handles work with files. */

#include "FileHandler.h"

/* Opens the file, and returns true upon success. If the directories don't exist, they will be created. */
bool FileHandler::openFile(const std::string& fileDestination, std::fstream& thisFile, bool writeFlag)
{
	std::filesystem::path pathToCheck = fileDestination;
	try {
		std::filesystem::create_directories(pathToCheck.parent_path()); 
		auto flags = writeFlag ? (std::fstream::binary | std::fstream::out) : (std::fstream::binary | std::fstream::in);
		thisFile.open(fileDestination.c_str(), flags);
		return thisFile.is_open();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return false;
	}
}

/* Closes a file, and returns true upon success. */
bool FileHandler::closeFile(std::fstream& thisFile)
{
	try {
		thisFile.close();
		return true;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return false;
	}
}

/* Reads the contents of a file into the payload buffer */
bool FileHandler::readFileIntoPayload(std::fstream& thisFile, char* payload, uint32_t count)
{
	try {
		thisFile.read(payload, count);
		return true;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return false;
	}
	return false;
}


/* Writes content into a file. fstream object is received, so the calling function is responsible for opening. */
bool FileHandler::writeToFile(std::fstream& thisFile, const char* content, uint32_t size)
{
	try {
		thisFile.write(content, size);
		return true;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return false;
	}
}

/* Deletes requested file. Calling function is responsible for ensuring that the file exists. 
If any errors occur, returns false. */
bool FileHandler::deleteFile(const std::string& fileDelete)
{
	try {		
		std::filesystem::path filePath = fileDelete;
		return std::filesystem::remove(filePath);		
	}
	catch (std::exception&) {
		return false;
	}	
}

/* Updates fileList with the files in the folder. If no folder exists, or no files exists, returns false. */
bool FileHandler::updateFileList(const std::string& folderDestination, std::string& fileList)
{
	try {
		std::filesystem::path folder = folderDestination;
		if (!std::filesystem::is_directory(folder))
			return false;

		for (const auto& file : std::filesystem::directory_iterator(folder)) 
			fileList.append(file.path().filename().string() + "\n");		

		return true;
	}
	catch (std::exception&) {
		return false;
	}
		

}

/* Returns true if fileDestination exists on the server, otherwise returns false. */
bool FileHandler::isExistent(const std::string& fileDestination)
{
	std::filesystem::path pathToCheck = fileDestination;
	return std::filesystem::exists(fileDestination);
}

/* Returns the size of file received. This function assumes that 4 bytes are enough to store the size. */
uint32_t FileHandler::getFileSize(const std::string& fileDestination)
{
	std::filesystem::path pathToCheck = fileDestination;
	return std::filesystem::file_size(pathToCheck); 
}
