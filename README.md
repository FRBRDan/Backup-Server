# C++ and Python Project: Backup Server & Client

## About

In this project I implemented a server software in C++ for backing up and retrieving files, as well as a client software in Python that works with it.

## Structure

- `main.cpp` - main function definition.
- `Server.cpp` - handles the logic of the server, essentially serves as the "back-end".
- `FileHandler.cpp` - handles all operations on files such as writing, reading, creation of directories, etc.
- `Log.cpp` - logs messages to the server prompt.

## Protocol used

All fields are positive (unsigned) and are represented in little endian byte ordering.

### Request

| Request | Field | Size | Meaning |
|---------|---------|---------|---------|
| Header | user id | 4 bytes | Represents the user |
| | version | 1 byte | Client's version number |
| | op | 1 byte | Request code |
| | name_len | 2 bytes | Length of file name |
| | filename | changes | Filename excluding null terminator |
| Payload | size | 4 bytes | Size of sent file |
| | payload | changes | Content of file (binary) |

| Op | Meaning | Remarks |
|---------|---------|---------|
|100| Backup file | All fields are full |
| 200 | Retrieve file | Payload and size fields don't exist |
| 201 | Delete file | Payload and size fields don't exist |
| 202 | Request for a list of all the client's files | name_len, filename, size, payload fields don't exist |

### Response

| Response | Field | Size | Meaning |
|---------|---------|---------|---------|
| Header | version | 1 byte | Client's version number |
| | status | 2 bytes | Response status |
| | name_len | 2 bytes | Length of file name |
| | filename | changes | Filename excluding null terminator |
| Payload | size | 4 bytes | Size of sent file |
| | payload | changes | Content of file (binary) |

| Status | Meaning | Remarks |
|---------|---------|---------|
| 210 | Success | File found and resoted. All fields are full |
| 211 | Success | List of files sent to client. Payload and size fields don't exist |
| 212 | Success | Upload/Deletion occurred. Payload and size fields don't exist |
| 1001 | Error | File doesn't exist. Payload and size don't exist |
| 1002 | Error | No files on server for this user. Only version and status fields exist |
| 1003 | Error | General error. Only version and status fields exist |

## The Algorithm

The server receives a request and begins to parse it according to the fields above. Both the client and the server have to parse the data in the same way, according to the protocol above. If the payload exceeds the size of one packet, then it is sent through multiple packets using fragmentation. The algorithm also allows for simultaneous connections using threading. Data security was also considered at all times.

## Example

### Server side 
![image](https://user-images.githubusercontent.com/72566563/186453021-eb279a0f-8cfe-444a-8ee4-e0eb05047d2e.png)


### Client side
![image](https://user-images.githubusercontent.com/72566563/186453115-886bc44e-0555-4559-814a-42f1f7481d47.png)

