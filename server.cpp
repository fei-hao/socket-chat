#include "stdafx.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")

#define IP_ADDRESS "192.168.1.141"
#define DEFAULT_PORT "4869"
#define BUFFER_SIZE 1024

struct ClientType {
	int id;
	SOCKET socket;
};

const char OPTION_VALUE = 1;
const int MAX_CLIENTS = 5;

//Function Prototypes
int processClient(ClientType &newClient, std::vector<ClientType> &clientsArray, std::thread &thread);
int main();

int processClient(ClientType &newClient, std::vector<ClientType> &clientsArray, std::thread &thread) {
	std::string message = "";
	char tempMessage[BUFFER_SIZE] = "";

	//Session
	while (1) {
		memset(tempMessage, 0, BUFFER_SIZE);

		if (newClient.socket != 0) {
			int iResult = recv(newClient.socket, tempMessage, BUFFER_SIZE, 0);

			if (iResult != SOCKET_ERROR) {
				if (strcmp("", tempMessage))
					message = "Client #" + std::to_string(newClient.id) + ": " + tempMessage;

				std::cout << message.c_str() << std::endl;

				//Broadcast that message to the other clients
				for (int i = 0; i < MAX_CLIENTS; i++) {
					if (clientsArray[i].socket != INVALID_SOCKET)
						if (newClient.id != i)
							iResult = send(clientsArray[i].socket, message.c_str(), strlen(message.c_str()), 0);
				}
			}
			else {
				message = "Client #" + std::to_string(newClient.id) + " Disconnected";

				std::cout << message << std::endl;

				closesocket(newClient.socket);
				closesocket(clientsArray[newClient.id].socket);
				clientsArray[newClient.id].socket = INVALID_SOCKET;

				//Broadcast the disconnection message to the other clients
				for (int i = 0; i < MAX_CLIENTS; i++) {
					if (clientsArray[i].socket != INVALID_SOCKET)
						iResult = send(clientsArray[i].socket, message.c_str(), strlen(message.c_str()), 0);
				}

				break;
			}
		}
	} //end while

	thread.detach();

	return 0;
}

int main() {
	WSADATA wsaData;
	struct addrinfo hints;
	struct addrinfo *server = NULL;
	SOCKET serverSocket = INVALID_SOCKET;
	std::string message = "";
	std::vector<ClientType> client(MAX_CLIENTS);
	int numberOfClients = 0;
	int tempId = -1;
	std::thread myThreads[MAX_CLIENTS];

	//Initialize Winsock
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	//Setup hints
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Setup Server
	getaddrinfo(static_cast<PCSTR>(IP_ADDRESS), DEFAULT_PORT, &hints, &server);

	//Create a listening socket for connecting to server
	serverSocket = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

	//Setup socket options
	setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &OPTION_VALUE, sizeof(int)); 
	setsockopt(serverSocket, IPPROTO_TCP, TCP_NODELAY, &OPTION_VALUE, sizeof(int)); 
	bind(serverSocket, server->ai_addr, (int)server->ai_addrlen);

	//Listen for incoming connections.
	std::cout << "Listening..." << std::endl;
	listen(serverSocket, SOMAXCONN);

	//Initialize the client list
	for (int i = 0; i < MAX_CLIENTS; i++) {
		client[i] = { -1, INVALID_SOCKET };
	}

	while (1) {

		SOCKET incoming = INVALID_SOCKET;
		incoming = accept(serverSocket, NULL, NULL);

		if (incoming == INVALID_SOCKET) continue;

		//Reset the number of clients
		numberOfClients = -1;

		//Create a temporary id for the next client
		tempId = -1;
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (client[i].socket == INVALID_SOCKET && tempId == -1) {
				client[i].socket = incoming;
				client[i].id = i;
				tempId = i;
			}

			if (client[i].socket != INVALID_SOCKET)
				numberOfClients++;

			//std::cout << client[i].socket << std::endl;
		}

		if (tempId != -1) {
			//Send the id to that client
			std::cout << "Client #" << client[tempId].id << " Accepted" << std::endl;
			message = std::to_string(client[tempId].id);
			send(client[tempId].socket, message.c_str(), strlen(message.c_str()), 0);

			//Create a thread process for that client
			myThreads[tempId] = std::thread(processClient, std::ref(client[tempId]), std::ref(client), std::ref(myThreads[tempId]));
		}
		else {
			message = "Server is full";
			send(incoming, message.c_str(), strlen(message.c_str()), 0);
			std::cout << message << std::endl;
		}
	} //end while


	  //Close listening socket
	closesocket(serverSocket);

	//Close client socket
	for (int i = 0; i < MAX_CLIENTS; i++) {
		myThreads[i].detach();
		closesocket(client[i].socket);
	}

	//Clean up Winsock
	WSACleanup();
	std::cout << "Program has ended successfully" << std::endl;

	system("pause");
	return 0;
}