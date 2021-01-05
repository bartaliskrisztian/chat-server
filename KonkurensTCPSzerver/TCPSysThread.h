#pragma once
#include <stdio.h>
#include "winsock2.h"
#include <Ws2tcpip.h>
#include <string>
#include <list>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#include "SysThread.h"

class TCPSysThread : public SysThread
{
private:
	SOCKET AcceptSocket;
	CRITICAL_SECTION* critical_section;
	std::list<TCPSysThread*>* threads;
	std::vector<std::string>* clients;
	std::string clientName;
public:

	TCPSysThread(SOCKET, std::list<TCPSysThread*>*, CRITICAL_SECTION*, std::vector<std::string>*);

	SOCKET getSocket();
	virtual void run();
	void sendEveryone(char*, int);
	void clientConnected(char*);
	void clientDisconnected();
	void takenUsername();
	void sendClientList();
};

