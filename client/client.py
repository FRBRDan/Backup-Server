from ensurepip import version
import socket
import random
from venv import create
from constants import *
import os
import struct


def exitWithError(error):
    """ Exits the program with a given error to be displayed. """
    print("Error: ", error)
    exit(1)


def createSocket(HOST, PORT):
    """
    Creates a socket with retreived server info.
    Calling function is responsible for closing the socket.
    """
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        return s
    except Exception as e:
        exitWithError(f"Exception occured while creating socket: {e}")


def createUniqueNumber():
    """ Creates a random number up to 4 bytes. """
    return random.randint(1, 0xFFFFFFFF)


def getServerFromFile(file):
    """ Retreives server information from file 'server.info' and returns HOST IP and PORT. """
    try:
        with open(file, 'r') as f:
            HOST, PORT = f.read().split(":")
        return HOST, int(PORT)
    except Exception as e:
        exitWithError(f'Exception occurred while retreiving server info: {e}')


def getFilesFromFile(file):
    """ Returns a list of files in received file. """
    try:        
        with open(file, 'r') as f:
            fileList = f.read().split('\n')
        return fileList

    except Exception as e:
        exitWithError(f'Exception occurred while retreiving file names: {e}')


def sendPacket(socket, buffer):
    """ The function pads the buffer with \0 and sends it over the socket. """
    if len(buffer) < PACKET_SIZE:
        buffer += bytearray(PACKET_SIZE - len(buffer))  # Pad with \0

    socket.send(buffer)


def createDirectory(directory):
    """ If directory doesn't exist, it is created. """
    if not os.path.exists(directory):
        os.mkdir(directory)


def requestFileUpload(UserID, version, filename, HOST, PORT):
    """ Uploads a desired file to the server """
    try:
        payloadSize = os.path.getsize(filename)
        currRequest = Request(UserID, version, Op.UPLOAD_FILE, filename, payloadSize)
        f = open(filename, "rb") 

        remainingBytesInPacket = PACKET_SIZE - currRequest.offset()
        currRequest.payload = f.read(remainingBytesInPacket)

        sock = createSocket(HOST, PORT)  
        sendPacket(sock, currRequest.littleEndianPack())

        while True:
            tempPayload = f.read(PACKET_SIZE)
            if not tempPayload:
                break
            sendPacket(sock, tempPayload)

        f.close()

        receivedBuffer = sock.recv(PACKET_SIZE)   
        sock.close()

        receivedResponse = Response()
        receivedResponse.littleEndianUnpack(receivedBuffer)

        if (receivedResponse.confirm(Status.FILE_SUCCESS_UP_DEL)):
            print(f'File {filename} was successfully uploaded to the server.')

    except Exception as e:
        print(f'File upload failed. Exception: {e}.')

  


def requestFileDownload(UserID, version, filename, HOST, PORT):
    """ Downloads a desired file from the server """
    try:
        currRequest = Request(UserID, version, Op.DOWNLOAD_FILE, filename, 0)
        sock = createSocket(HOST, PORT)  
        sendPacket(sock, currRequest.littleEndianPack())

        receivedResponse = Response()
        receivedBuffer = sock.recv(PACKET_SIZE)
        receivedResponse.littleEndianUnpack(receivedBuffer)
        currPayloadSize = min(receivedResponse.payloadSize, PACKET_SIZE - receivedResponse.offset())

        if not receivedResponse.confirm(Status.FILE_SUCCESS_DOWNLOAD):
            sock.close()
            return

        createDirectory('backup')

        f = open('backup\\' + filename, 'wb')
        f.write(receivedResponse.payload)

        sizeLeft = receivedResponse.payloadSize - currPayloadSize

        while sizeLeft > 0:
            tempPayload = sock.recv(PACKET_SIZE)
            currPayloadSize = min(sizeLeft, PACKET_SIZE)            
            f.write(tempPayload[:currPayloadSize])
            sizeLeft -= currPayloadSize

        f.close()
        sock.close()
        
        print(f'File {filename} was successfully downloaded from the server.')


    except Exception as e:
        print(f'Download file failed. Exception: {e}.')
        
    
    
def requestFileDelete(UserID, version, filename, HOST, PORT):
    """ Deletes a desired file from the server """
    try:
        currRequest = Request(UserID, version, Op.DELETE_FILE, filename, 0)
        sock = createSocket(HOST, PORT)  
        sendPacket(sock, currRequest.littleEndianPack())

        receivedResponse = Response()
        receivedBuffer = sock.recv(PACKET_SIZE)
        receivedResponse.littleEndianUnpack(receivedBuffer)

        # Add validation of Code.
        # Add Creation of backup folder if doesn't exist

        sock.close()

        if (receivedResponse.confirm(Status.FILE_SUCCESS_UP_DEL)):
            print(f'File {filename} was successfully DELETED from the server.')


    except Exception as e:
        print(f'File deletion failed. Exception: {e}.')
    


def requestAllFiles(UserID, version, HOST, PORT):
    """ Returns all files of user from the server """
    try:
        currRequest = Request(UserID, version, Op.REQUEST_ALL_FILES, '', 0)
        sock = createSocket(HOST, PORT)  
        sendPacket(sock, currRequest.littleEndianPack())

        currResponse = Response()
        receivedBuffer = sock.recv(PACKET_SIZE)
        currResponse.littleEndianUnpack(receivedBuffer)
        currPayloadSize = min(currResponse.payloadSize, PACKET_SIZE - currResponse.offset())

        sizeLeft = currResponse.payloadSize - currPayloadSize

        while sizeLeft > 0:
            tempPayload = sock.recv(PACKET_SIZE)
            currPayloadSize = min(sizeLeft, PACKET_SIZE)            
            currResponse.payload += tempPayload[:currPayloadSize]
            sizeLeft -= currPayloadSize

        sock.close()
        
        if currResponse.confirm(Status.FILES_LIST_SENT):
            fileList = currResponse.payload.decode('utf-8')
            print(f'The files stored on server for user {UserID} are: \n{fileList}')


    except Exception as e:
        print(f'Files list request failed. Exception: {e}.')
    


class Request:
    def __init__(self, UserID, version, op, filename, payloadSize):
        self.UserID = UserID
        self.version = version
        self.op = op.value
        self.nameLength = len(filename)
        self.filename = bytes(filename, 'utf-8')
        self.payloadSize = payloadSize
        self.payload = b''

    def offset(self):
        """ Returns offset for payload. """
        return self.nameLength + REQUEST_OFFSET  

    def littleEndianPack(self):
        """ Packs the data into a struct according to the server's protocol. """
        packedData = struct.pack(f'<IBBH{self.nameLength}sI', self.UserID, self.version, self.op, self.nameLength, self.filename, self.payloadSize)
        packedData += self.payload
        return packedData

    
class Response:
    def __init__(self):
        self.version = 0
        self.status = 0
        self.nameLength = 0
        self.filename = b''
        self.payloadSize = 0
        self.payload = b''
    
    def offset(self):
        """ Returns offset for payload. """
        return self.nameLength + RESPONSE_OFFSET  

    def littleEndianUnpack(self, buffer):
        """ Unpacks binary data received into the correct fields """
        try:
            INT_SIZE = 4
            currOffset = 5  # 3 first fields take 5 bytes
            self.version, self.status, self.nameLength = struct.unpack('<BHH', buffer[:currOffset])
            self.filename = struct.unpack(f'<{self.nameLength}s', buffer[currOffset:currOffset + self.nameLength])[0].decode('utf-8')
            currOffset += self.nameLength
            self.payloadSize = struct.unpack('<I', buffer[currOffset:currOffset + INT_SIZE])[0]
            currOffset += INT_SIZE
            infoToExtract = min(PACKET_SIZE - currOffset, self.payloadSize)
            self.payload = struct.unpack(f'<{infoToExtract}s', buffer[currOffset:currOffset + infoToExtract])[0]

        except Exception as e:
            print(e)

    def confirm(self, desiredStatus):
        """ 
        This function will confirm that the response is OK - 210/211/212.
        To make this generic, errors will be checked only.
        """
        if self.status == desiredStatus.value:
            return True
        elif self.status == Status.FILE_NOT_FOUND.value:
            print(f'Error: File {self.filename} not found. ')
        elif self.status == Status.NO_FILES_FOR_USER.value:
            print(f'Error: There are no files for the user on the server.')
        elif self.status == Status.GENERAL_ERROR.value:
            print(f'Error: General Error occurred.')       
        elif self.status is None or self.status != desiredStatus.value:            
            print('Error: Invalid status received.', self.status)        
        return False


def main():
    HOST, PORT = getServerFromFile(SERVER_INFO)
    fileList = getFilesFromFile(BACKUP_INFO)
    UserID = createUniqueNumber()       
    
    requestAllFiles(UserID, CLIENT_VER, HOST, PORT)
    requestFileUpload(UserID, CLIENT_VER, fileList[0], HOST, PORT)
    requestFileUpload(UserID, CLIENT_VER, fileList[1], HOST, PORT)
    requestAllFiles(UserID, CLIENT_VER, HOST, PORT)
    requestFileDownload(UserID, CLIENT_VER, fileList[0], HOST, PORT)
    requestFileDelete(UserID, CLIENT_VER, fileList[0], HOST, PORT)
    requestFileDownload(UserID, CLIENT_VER, fileList[0], HOST, PORT)

    requestFileDownload(UserID, CLIENT_VER, fileList[1], HOST, PORT)


if __name__ == '__main__':
    main()
