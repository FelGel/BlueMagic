// PositioningAlgorithms.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "PositioningAlgorithms.h"
#include "Common/Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int /*argc*/, TCHAR* /*argv*/[], TCHAR* /*envp*/[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
	}

	return nRetCode;
}


/*static*/ void CPositioningAlgorithms::SetTheLogManagerDLL(CLogManager *OtherTheLogManager)
{
	SetTheLogManager(OtherTheLogManager);
}


/*static */void CPositioningAlgorithms::SetLogEventOutputDLL(LogEventOutput TheOutput, bool LogMessagesToFile, GetLogEventOutputSeverity GetSeverity, const char* LogMessagesBaseName)
{
	SetLogEventOutput(TheOutput, LogMessagesToFile, GetSeverity, LogMessagesBaseName);
	LogEvent(LE_NOTICE, __FUNCTION__ ": PositioningAlgorithms DLL log initialized");
}

/*static*/ void CPositioningAlgorithms::SetConfigFileNameDLL(const char* ConfigFileName)
{
	SetConfigFileName(ConfigFileName);
	SetConfigWritebackMode(true);
}

/*static*/ void CPositioningAlgorithms::SetCrashFileNameDLL(const char* ApplicationName)
{
	std::string CrashFileName = std::string(GetProgramPath()) + ApplicationName + ".crash";
	CExceptionReport::GetTheExceptionReport().Init(CrashFileName.c_str());
}