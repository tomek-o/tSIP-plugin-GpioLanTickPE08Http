//---------------------------------------------------------------------
#pragma hdrstop
#include <vcl.h>

#include "FormAbout.h"
#include "Utils.h"
//--------------------------------------------------------------------- 
#pragma resource "*.dfm"
TfrmAbout *frmAbout;

//i.e. GetFileVer(Application->ExeName)
AnsiString GetFileVer(AnsiString FileName)
{
    AnsiString asVer="";
    VS_FIXEDFILEINFO *pVsInfo;
    unsigned int iFileInfoSize = sizeof( VS_FIXEDFILEINFO );
    
    int iVerInfoSize = GetFileVersionInfoSize(FileName.c_str(), NULL);
    if (iVerInfoSize!= 0)
    {
        char *pBuf = new char[iVerInfoSize];
        if (GetFileVersionInfo(FileName.c_str(),0, iVerInfoSize, pBuf ) )
        {
            if (VerQueryValue(pBuf, "\\",(void **)&pVsInfo,&iFileInfoSize))
            {
                asVer  = IntToStr( HIWORD(pVsInfo->dwFileVersionMS) )+".";
                asVer += IntToStr( LOWORD(pVsInfo->dwFileVersionMS) )+".";
                asVer += IntToStr( HIWORD(pVsInfo->dwFileVersionLS) )+".";
                asVer += IntToStr( LOWORD(pVsInfo->dwFileVersionLS) );
            }
        }
        delete [] pBuf;
    }
    return asVer;
}

//--------------------------------------------------------------------- 
__fastcall TfrmAbout::TfrmAbout(TComponent* AOwner)
	: TForm(AOwner)
{
	lblVersion->Caption = GetFileVer(Utils::GetDllPath().c_str());
	lblBuildTimestamp->Caption = (AnsiString)__DATE__ + ", " __TIME__;
}
//---------------------------------------------------------------------

