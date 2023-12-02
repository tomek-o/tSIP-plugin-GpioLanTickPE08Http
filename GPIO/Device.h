#ifndef DeviceH
#define DeviceH

#include <string>
#include <deque>
#include <winsock2.h>
#include "Mutex.h"

class Device {
public:
	/** \return non-zero on error */
	typedef int (*pfcnReceiveBuf)(unsigned char byte, void* opaque);
	typedef void (*pfcnPoll)(void* opaque);
private:
	std::string sAddress;
	int port;
	bool connectRequest;
	bool connected;
	bool connectionLost;

	SOCKET socketHandle;
	HANDLE workerThread;
	HANDLE keepaliveThread;
	bool terminate;

	volatile bool keepaliveTerminated;
	friend DWORD WINAPI ConnKeepaliveThreadProc(LPVOID lpParameter);
	void ConnKeepaliveProc(void);

	Mutex mutex;

	int ProcessByte(unsigned char byte);
	int SendBuf(std::string buf);
	int ConnClose(void);
	int waitForReply;

	std::deque<std::string> bufQueue;
	Mutex mutexQueue;
	pfcnReceiveBuf onReceiveCb;
	void* onReceiveOpaque;

	pfcnPoll onPoll;
	void* onPollOpaque;

	int keepAlivePeriod;	//ms;	 no keepalive if <= 0
	std::string keepAliveBuf;
	std::string initBuf;

	int Connect(void);

public:
	Device(void);
	void SetReceiveBufCb(pfcnReceiveBuf onReceiveCb, void* onReceiveOpaque) {
		this->onReceiveCb = onReceiveCb;
		this->onReceiveOpaque = onReceiveOpaque;
	}
	void SetPollCb(pfcnPoll onPollCb, void* onPollOpaque) {
		this->onPoll = onPollCb;
		this->onPollOpaque = onPollOpaque;
	}
	int Configure(std::string sAddress, int port, std::string initBuf, int keepAlivePeriod, std::string keepAliveBuf);
	int Start(void);
	int Disconnect(void);
	bool isConnected(void) const {
		return connected;
	}
	bool connLost(void) {
		bool val = connectionLost;
		connectionLost = false;
		return val;
	}
	int EnqueueBuf(std::string buf);
};

#endif
