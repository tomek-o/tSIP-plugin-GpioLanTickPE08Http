//---------------------------------------------------------------------------

#ifndef PhoneStateH
#define PhoneStateH

#include <string>

//---------------------------------------------------------------------------
struct PhoneState
{
	bool registered;
	bool paging;
	int callState;
	std::string display;
	PhoneState(void):
		registered(false),
		paging(false),
		callState(0)
	{}
};

extern PhoneState phoneState;

#endif
