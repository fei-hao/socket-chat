#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

#pragma comment (lib, "Ws2_32.lib")
        
#define IP_ADDRESS "192.168.1.141"
#define DEFAULT_PORT "4869"
#define BUFFER_SIZE 1024 

struct ClientType {
	SOCKET socket;
	int id;
	char receivedMessage[BUFFER_SIZE];
};

int processClient(ClientType &newClient);
int main();

int processClient(ClientType &newClient) {
	while (1) {
		memset(newClient.receivedMessage, 0, BUFFER_SIZE);

		if (newClient.socket != 0) {
			int iResult = recv(newClient.socket, newClient.receivedMessage, BUFFER_SIZE, 0);

			if (iResult != SOCKET_ERROR)
				cout << newClient.receivedMessage << endl;
			else {
				cout << "recv() failed: " << WSAGetLastError() << endl;
				break;
			}
		}
	}

	if (WSAGetLastError() == WSAECONNRESET)
		cout << "The server has disconnected" << endl;

	return 0;
}

int main() {
	WSAData wsadata;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	string sentMessage = "";
	ClientType client = { INVALID_SOCKET, -1, "" };
	int iResult = 0;
	string message;

	cout << "Starting Client...\n";

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsadata);
	if (iResult != 0) {
		cout << "WSAStartup() failed with error: " << iResult << endl;
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	cout << "Connecting...\n";

	// Resolve the server address and port
	iResult = getaddrinfo(static_cast<PCSTR>(IP_ADDRESS), DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		cout << "getaddrinfo() failed with error: " << iResult << endl;
		WSACleanup();
		system("pause");
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		client.socket = socket(ptr->ai_family, ptr->ai_socktype,
							   ptr->ai_protocol);
		if (client.socket == INVALID_SOCKET) {
			cout << "socket() failed with error: " << WSAGetLastError() << endl;
			WSACleanup();
			system("pause");
			return 1;
		}

		// Connect to server.
		iResult = connect(client.socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(client.socket);
			client.socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (client.socket == INVALID_SOCKET) {
		cout << "Unable to connect to server!" << endl;
		WSACleanup();
		system("pause");
		return 1;
	}

	cout << "Successfully Connected" << endl;

	//Obtain id from server for this client;
	recv(client.socket, client.receivedMessage, BUFFER_SIZE, 0);
	message = client.receivedMessage;

	if (message != "Server is full") {
		client.id = atoi(client.receivedMessage);

		thread my_thread(processClient, client);

		while (1) {
			getline(cin, sentMessage);
			iResult = send(client.socket, sentMessage.c_str(), strlen(sentMessage.c_str()), 0);

			if (iResult <= 0) {
				cout << "send() failed: " << WSAGetLastError() << endl;
				break;
			}
		}

		//Shutdown the connection since no more data will be sent
		my_thread.detach();
	}
	else
		cout << client.receivedMessage << endl;

	cout << "Shutting down socket..." << endl;
	iResult = shutdown(client.socket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown() failed with error: " << WSAGetLastError() << endl;
		closesocket(client.socket);
		WSACleanup();
		system("pause");
		return 1;
	}

	closesocket(client.socket);
	WSACleanup();
	system("pause");
	return 0;
}