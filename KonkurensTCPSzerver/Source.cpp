#include <stdio.h>
#include "winsock2.h"
#include <Ws2tcpip.h>
#include "TCPSysThread.h"
#include <list>
#include <vector>

#pragma comment(lib, "ws2_32.lib")


using namespace std;

int main() {

	WSADATA wsaData;
	int result;

	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR)
		printf("Hiba a WSAStartup() –nál\n");

	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Hiba a socket inicializálásánál a következõ hibakóddal: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &service.sin_addr);
	service.sin_port = htons(13000);

	if (bind(ListenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		printf("bind() failed.\n");
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(ListenSocket, 1) == SOCKET_ERROR) {
		printf("Error listening on socket.\n");
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	SOCKET AcceptSocket;
	printf("Varakozas a kliensek kapcsolodasara...\n");

	CRITICAL_SECTION critical_section;
	InitializeCriticalSection(&critical_section);

	list<TCPSysThread*> threads;
	vector<std::string> clients;

	while (1) {

		char clientName[25];

		AcceptSocket = accept(ListenSocket, NULL, NULL);
		if (AcceptSocket == INVALID_SOCKET) {
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			break;
		}

		else
		{  
			/*
			
			*/
		}

		TCPSysThread* thread = new TCPSysThread(AcceptSocket, &threads, &critical_section, &clients);

		if (!thread->start()) {
			printf("Sikertelen a szal inditasa.\n");
			return 1;
		}
	}

	for (auto& t : threads) {
		delete(t);
	}

	printf("Socket bezarasa.\n");
	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}