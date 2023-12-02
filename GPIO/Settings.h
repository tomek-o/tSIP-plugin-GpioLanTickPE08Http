/** \file
*/
//---------------------------------------------------------------------------

#ifndef SettingsH
#define SettingsH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <deque>

class Settings
{
public:
	bool Read(AnsiString asFileName);
	bool Write(AnsiString asFileName);
	struct _frmMain
	{
		int iPosX, iPosY;				///< main window coordinates
		int iHeight, iWidth;			///< main window size
		bool bWindowMaximized;			///< is main window maximize?
		bool bAlwaysOnTop;
	} frmMain;
	struct _device
	{
		enum { DEFAULT_PORT = 80 };
		AnsiString asAddress;
		int port;
		_device(void):
			asAddress("192.168.1.93"),
			port(DEFAULT_PORT)
		{}
	} Device;
	struct _Logging
	{
		bool bLogToFile;
		unsigned int iMaxUiLogLines;
	} Logging;
};

extern Settings appSettings;

#endif
