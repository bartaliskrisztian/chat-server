#include "TCPSysThread.h"
#include <string.h>
#include <algorithm>


TCPSysThread::TCPSysThread(SOCKET socket, std::list<TCPSysThread*>* threads, CRITICAL_SECTION* cs, std::vector<std::string>* clients) {
	AcceptSocket = socket;
	this->threads = threads;
	this->critical_section = cs;
	this->clients = clients;
	this->clientName = "";

	int result;
	WSADATA wsaData;
	
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		printf("Hiba a WSAStartup() �n�l\n");
	}
}

void TCPSysThread::run() {
	const int recBufLen = 1024;
	char recBuf[recBufLen];
	int result;

	EnterCriticalSection(critical_section);
	threads->push_back(this);
	LeaveCriticalSection(critical_section);

	while (true) {

		// kliens �ltal k�ld�tt �zenet fogad�sa
		result = recv(this->AcceptSocket, recBuf, recBufLen - 1, 0);
		if (result == SOCKET_ERROR) {
			clientDisconnected();
			break;
		}

		if (result == 0) {
			clientDisconnected();
			break;
		}

		recBuf[result] = '\0';
		
		// az �zenet t�pusa
		char packetType = recBuf[0];

		switch (packetType) {
			// amikor csatlakozik egy kliens
			case '0': {
				std::string clientName = "";
				for (int i = 1; i < result; ++i) {
					clientName += recBuf[i];
				}

				auto position = std::find(clients->begin(), clients->end(), clientName);
				// ha m�g nem l�tezik az adott username
				if (position != clients->end()) {
					takenUsername();
				}
				else {
					this->clientName = clientName;

					EnterCriticalSection(critical_section);
					clients->push_back(clientName);
					LeaveCriticalSection(critical_section);

					// elk�ldj�k mindenkinek a csatlakozott kliens nev�t
					clientConnected(recBuf);
					Sleep(200);
					sendClientList();
				}

				break;
			}
			// mindenkinek k�ld�s
			case '2': {
				std::string clientName = "";
				std::string message = "";
				int i;
				for (i = 1; i < result; ++i) {
					if (recBuf[i] == '^') {
						++i;
						break;
					}
					clientName += recBuf[i];
				}
				for (int j = i; j < result; ++j) {
					message += recBuf[j];
				}
				printf("%s: %s\n", clientName.c_str(), message.c_str());
				sendEveryone(recBuf, result);
				break;
			}
			// priv�t k�ld�s
			case '5': {
				std::string clientName = "";
				std::string friendName = "";
				std::string message = "";
				int i, j, k;
				for (i = 1; i < result; ++i) {
					if (recBuf[i] == '^') {
						++i;
						break;
					}
					clientName += recBuf[i];
				}
				for (j = i; j < result; ++j) {
					if (recBuf[j] == '^') {
						++j;
						break;
					}
					friendName += recBuf[j];
				}
				for (k = j; k < result; ++k) {
					message += recBuf[k];
				}
				printf("%s -> %s: %s\n", clientName.c_str(), friendName.c_str(), message.c_str());
				privateSend(recBuf, result);
				break;
			}
		}
	}
}


SOCKET TCPSysThread::getSocket() {
	return this->AcceptSocket;
}

// f�ggv�ny a mindenki k�ld�s sz�m�ra
void TCPSysThread::sendEveryone(char* recBuf, int dataLen) {

	int result;

	for (auto& t : *threads) {
		if (!t->isExited()) {
			result = send(t->getSocket(), recBuf, dataLen, 0);
			if (result == SOCKET_ERROR) {
				printf("Hiba a k�ldesnel a kovetkezo hibakoddal: %d\n", WSAGetLastError());
				break;
			}
		}
		else {
			EnterCriticalSection(critical_section);
			threads->remove(t);
			LeaveCriticalSection(critical_section);
		}
	}
}

// amikor egy kliens csatlakozik, elk�ldj�k mindenkinek a nev�t
void TCPSysThread::clientConnected(char* recBuf) {
	
	int dataLen = strlen(recBuf) + 1;
	int result;
	for (auto& t : *threads) {
		if (!t->isExited()) {
			result = send(t->getSocket(), recBuf, dataLen, 0);
			if (result == SOCKET_ERROR) {
				printf("Hiba a k�ldesnel a kovetkezo hibakoddal: %d\n", WSAGetLastError());
				break;
			}
		}
		else {
			EnterCriticalSection(critical_section);
			threads->remove(t);
			LeaveCriticalSection(critical_section);
		}
	}

	printf("'%s' csatlakozott\n", this->clientName.c_str());
}

// amikor egy kliens kil�p, elk�ldj�k mindenkinek a nev�t
void TCPSysThread::clientDisconnected() {

	printf("'%s' kilepett.\n", clientName.c_str());

	EnterCriticalSection(critical_section);
	if (!this->isExited()) {
		auto position = std::find(clients->begin(), clients->end(), clientName);
		if (position != clients->end()) {
			clients->erase(position);
		}
	}
	LeaveCriticalSection(critical_section);

	char recBuf[50];
	std::string message = "";
	message += '1';
	message += clientName;

	strcpy_s(recBuf, message.c_str());
	int dataLen = strlen(recBuf) + 1;
	int result;

	for (auto& t : *threads) {
		if (!t->isExited()) {
			result = send(t->getSocket(), recBuf, dataLen, 0);
			if (result == SOCKET_ERROR) {
				printf("Hiba a k�ldesnel a kovetkezo hibakoddal: %d\n", WSAGetLastError());
				break;
			}
		}
		else {
			EnterCriticalSection(critical_section);
			threads->remove(t);
			LeaveCriticalSection(critical_section);
		}
	}

	EnterCriticalSection(critical_section);
	if (!this->isExited()) {
		threads->remove(this);
	}
	LeaveCriticalSection(critical_section);

	closesocket(this->AcceptSocket);
}

// ha egy n�v foglalt, elk�ldj�k a hiba�zenetet a kliensnek
void TCPSysThread::takenUsername() {
	char recBuf[50];
	std::string message = "";
	message += '3';
	message += "A megadott nev foglalt";

	strcpy_s(recBuf, message.c_str());
	int dataLen = strlen(recBuf) + 1;
	int result;

	result = send(this->getSocket(), recBuf, dataLen, 0);
	if (result == SOCKET_ERROR) {
		printf("Hiba a k�ldesnel a kovetkezo hibakoddal: %d\n", WSAGetLastError());
		return;
	}

	EnterCriticalSection(critical_section);
	if (!this->isExited()) {
		threads->remove(this);
	}
	LeaveCriticalSection(critical_section);
}

// amikor egy kliens csatlakozik, elk�ldj�k neki a t�bbi felhaszn�l� neveit
void TCPSysThread::sendClientList() {
	char recBuf[1024];
	std::string message = "";
	message += '4';

	EnterCriticalSection(critical_section);
	for (int i = 0; i < clients->size(); ++i) {
		message += clients->at(i);
		message += '^';
	}

	strcpy_s(recBuf, message.c_str());
	int dataLen = strlen(recBuf) + 1;
	int result;

	result = send(this->getSocket(), recBuf, dataLen, 0);
	if (result == SOCKET_ERROR) {
		printf("Hiba a k�ldesnel a kovetkezo hibakoddal: %d\n", WSAGetLastError());
	}
	
	LeaveCriticalSection(critical_section);
}

void TCPSysThread::privateSend(char* recBuf, int dataLen) {

}