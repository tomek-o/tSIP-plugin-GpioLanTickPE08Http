//---------------------------------------------------------------------------


#pragma hdrstop

#include "GpioLantickPE08.h"
#include "PhoneLocal.h"
#include "Device.h"
#include "Utils.h"
#include <algorithm>
#include <stdio.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

#include "Log.h"


std::string varStateName;
std::string varConnName;

GpioLantick::GpioLantick(Device &device):
	device(device),
	connected(false),
	connectedInitialized(false),
	receivedCount(0),
	noRxCounter(0)
{
	std::string path = Utils::GetDllPath();
	std::string dllName = Utils::ExtractFileNameWithoutExtension(path);
	varStateName = dllName;
	#pragma warn -8091	// incorrectly issued by BDS2006
	std::transform(varStateName.begin(), varStateName.end(), varStateName.begin(), tolower);
	varConnName = varStateName;
	varStateName += "State";
	varConnName += "Conn";
}

int GpioLantick::OnDeviceBufRx(unsigned char byte)
{
	char str[16];
	sprintf(str, "%d", byte);
	SetVariable(varStateName.c_str(), str);
	LOG("Value: %d\n", byte);
	SetVariable(varConnName.c_str(), "1");
	noRxCounter = 0;	
	return 0;
}

void GpioLantick::OnPoll(void)
{
	noRxCounter++;
	if (noRxCounter == 1000)
	{
		SetVariable(varConnName.c_str(), "0");
		ClearVariable(varStateName.c_str());
	}
}
