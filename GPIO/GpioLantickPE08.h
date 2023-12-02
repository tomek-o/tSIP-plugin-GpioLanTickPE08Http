//---------------------------------------------------------------------------

#ifndef GpioLantickPE08H
#define GpioLantickPE08H
//---------------------------------------------------------------------------

#include <string>

class Device;

class GpioLantick
{
public:
	enum { KEEPALIVE_PERIOD = 10000 };	///< note initCmd must be also updated to maintain time margin
	GpioLantick(Device &device);
protected:
	Device &device;
	bool connected;
	bool connectedInitialized;
	int receivedCount;
	unsigned char rxBuf[16];
	int noRxCounter;

	friend int OnDeviceBufReceive(unsigned char byte, void* opaque);
	int OnDeviceBufRx(unsigned char byte);

	friend void OnPeriodicPoll(void* opaque);
	void OnPoll(void);
};

#endif
