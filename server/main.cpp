/* main.cpp */

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <thread>
#include "Server.h"

/* Handles the connection, creates a Server instance and passes the request on to the Server class.*/
void connectionHandler(SOCKET clientsocket) {
	Server serverHandler;	
	serverHandler.handleRequest(clientsocket);
	closesocket(clientsocket);
}

/* Initializes the server. */
void initializeAndRun() {
	std::cout << "Server is running.." << std::endl;

	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in sa = { 0 };
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(PORT);

	bind(sock, (struct sockaddr*)&sa, sizeof(sa));


	listen(sock, SOMAXCONN);
	std::cout << "Listening on port " << PORT << "\n" << std::endl;

	for (;;) {
		SOCKET clientsocket = accept(sock, NULL, NULL);
		std::thread ct(connectionHandler, clientsocket); // To allow for more connections at a time.
		ct.detach();		
	}
	closesocket(sock);

	WSACleanup();

}

/* Main. */
int main() {

	try {
		initializeAndRun();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
	
	return 0;
}

