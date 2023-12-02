#pragma hdrstop

#include "Device.h"
#include "Log.h"
#include "ScopedLock.h"
#include "Utils.h"
#include <assert.h>
#include <stdio.h>

#define TRACE LOG("%s/%04d", __FILE__, __LINE__);

DWORD WINAPI ConnKeepaliveThreadProc(LPVOID lpParameter)
{
	(reinterpret_cast<Device*>(lpParameter))->ConnKeepaliveProc();
	return 0;
}

Device::Device(void):
	port(49280),
	socketHandle(INVALID_SOCKET),
	terminate(false),
	workerThread(INVALID_HANDLE_VALUE),
	keepaliveThread(INVALID_HANDLE_VALUE),
	connectRequest(false),
	connected(false),
	connectionLost(false),
	waitForReply(-1),
	onReceiveCb(NULL),
	onReceiveOpaque(NULL),
	keepaliveTerminated(true),
	onPoll(NULL),
	onPollOpaque(NULL),
	keepAlivePeriod(-1)
{
}

int Device::Configure(std::string sAddress, int port, std::string initBuf, int keepAlivePeriod, std::string keepAliveBuf) {
	this->sAddress = sAddress;
	this->port = port;
	this->initBuf = initBuf;
	this->keepAlivePeriod = keepAlivePeriod;
	this->keepAliveBuf = keepAliveBuf;
	return 0;
}

int Device::Start(void) {
	ScopedLock<Mutex> lock(mutex);
	assert(onReceiveCb);
	connectRequest = true;
	terminate = false;

	waitForReply = -1;

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		LOG("WSAStartup failed: %d", iResult);
		return 1;
	}

	keepaliveTerminated = false;
	HANDLE keepaliveThread = CreateThread(NULL, 0, ConnKeepaliveThreadProc, this, CREATE_SUSPENDED, NULL);
	SetThreadPriority(keepaliveThread, THREAD_PRIORITY_NORMAL);
	this->keepaliveThread = keepaliveThread;
	ResumeThread(keepaliveThread);

	return 0;
}

int Device::Connect(void) {
	if (connected) {
		LOG("Device: Connect: already connected");
		return 0;
	}

	{
		ScopedLock<Mutex> lock(mutexQueue);
		bufQueue.clear();
	}

	struct sockaddr_in sa;
	struct hostent* hp;

	LOG("Device: Connect to %s:%d", sAddress.c_str(), port);

	hp = gethostbyname(sAddress.c_str());
	if (hp == NULL)
	{
		int wsaLasterror = WSAGetLastError();
		LOG("Failed to resolve host address, WSA error %u", wsaLasterror);
		return -2;
	}

	memset(&sa, 0, sizeof(sa));
	memcpy((char *)&sa.sin_addr, hp->h_addr, hp->h_length);
	sa.sin_family = hp->h_addrtype;
	sa.sin_port = htons((u_short)port);

    socketHandle = socket(hp->h_addrtype, SOCK_STREAM, 0);
    if (socketHandle == INVALID_SOCKET)
    {
		LOG("Failed to create socket");
		return -3;
    }
/*
	int rcvTimeout = 50;
	if (setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcvTimeout, sizeof(int)) == SOCKET_ERROR) {
		LOG(L"Failed to set socket receive timeout");
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
		return -3;
	}
*/
	int sndTimeout = 3000; // 3 s
	if (setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sndTimeout, sizeof(int)) == SOCKET_ERROR) {
		LOG("Failed to set socket send timeout");
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
		return -3;
	}

    if (connect(socketHandle, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR) 
    {
		int wsaLasterror = WSAGetLastError();
		LOG("Connection failed, WSA error %d", wsaLasterror);
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
        return -4;
    }
	LOG("Connected to device");

#if 0
	int rc = SendBuf(initBuf);

	if (rc) {
		LOG("Failed to send preamble commands to device");
		closesocket(socketHandle);
		socketHandle = INVALID_SOCKET;
		return -5;
	}
#endif	

	connected = true;

	return 0;
}

int Device::Disconnect(void) {
	LOG("Disconnect...");
	terminate = true;	
	{
		ScopedLock<Mutex> lock(mutex);

		connectRequest = false;
		LOG("Device: Closing connection");
		ConnClose();
	}

	LOG("Device: Waiting for ka thread termination");
	while (keepaliveTerminated == false) {
		Sleep(50);
	}
	LOG("Device: ka thread terminated");

	WSACleanup();

	return 0;
}

int Device::ConnClose(void) {
	if (socketHandle != INVALID_SOCKET ) {
		SOCKET s = socketHandle;
		socketHandle = INVALID_SOCKET;
		shutdown(s, SD_BOTH);	// doesn't break existing recv call itself
		closesocket(s);
	}
	connected = false;
	return 0;
}

int Device::ProcessByte(unsigned char byte) {
	//LOG("RX: %dB", buf.size());
	return onReceiveCb(byte, onReceiveOpaque);
}

int Device::SendBuf(std::string buf) {
	ScopedLock<Mutex> lock(mutex);
	if (socketHandle == INVALID_SOCKET) {
		return 1;
	}
	LOG("TX: %dB", buf.size());
	int rc = send(socketHandle, buf.data(), (int)buf.size() , 0);
	if (rc == SOCKET_ERROR) {
		LOG("Device: socket send error");
		connectionLost = true;
		ConnClose();
		return 2;
	}
	return 0;
}

void Device::ConnKeepaliveProc(void) {
	LOG("ConnKeepaliveProc Start");
	enum { HANDLE_PERIOD = 20 };
	
	int kaCounter = keepAlivePeriod/HANDLE_PERIOD;

	while (!terminate) {
		{
			if (keepAlivePeriod > 0 && ((++kaCounter) >= (keepAlivePeriod/HANDLE_PERIOD)) ) {
				ScopedLock<Mutex> lock(mutex);
				
				// "keepalive"
				kaCounter = 0;

				if (Connect() != 0) {
					Sleep(HANDLE_PERIOD);
					continue;
				}

				// Sending a HTTP-GET-Request to the Web Server
				char httpRequest[16*1024];
				snprintf(httpRequest, sizeof(httpRequest),
					"GET /stat.php HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
					sAddress.c_str());
				httpRequest[sizeof(httpRequest)-1] = '\0';

				int sentBytes = send(socketHandle, httpRequest, strlen(httpRequest),0);
				if (sentBytes < strlen(httpRequest) || sentBytes == SOCKET_ERROR)
				{
					LOG("Could not send the request to the Server");
					closesocket(socketHandle);
					socketHandle = INVALID_SOCKET;
					connected = false;
					Sleep(HANDLE_PERIOD);					
					continue;
				}

				char buffer[10000];
				ZeroMemory(buffer, sizeof(buffer));
				int dataLen;
				char *ptr = buffer;
				while ((dataLen = recv(socketHandle, ptr, sizeof(buffer) - (ptr-buffer) - 1, 0) > 0))
				{
					ptr += dataLen;
					Sleep(10);
				}
				buffer[sizeof(buffer)-1] = '\0';

				//LOG("RX buffer: %s\n", buffer);

				const char* p = strstr(buffer, "<in>");
				if (p != NULL) {
					p += strlen("<in>");
					unsigned char byte = 0;
					for (int i=0; i<8; i++) {
						if (p[i] == '1') {
							byte |= (1 << (7-i));
						} else if (p[i] == '0') {

						} else {
                        	break;
						}
					}
					ProcessByte(byte);
				} else {
					LOG("RX: <in> state not found\n");
				}

				closesocket(socketHandle);
				socketHandle = INVALID_SOCKET;
				connected = false;
			}
			if (onPoll) {
				onPoll(onPollOpaque);
			}
		}
		Sleep(HANDLE_PERIOD);
	}
	LOG("ConnKeepaliveProc Exit");
	keepaliveTerminated = true;
}

int Device::EnqueueBuf(std::string buf) {
	ScopedLock<Mutex> lock(mutexQueue);
	bufQueue.push_back(buf);
	return 0;
}

