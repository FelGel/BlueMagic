// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BLUEMAGICCOMMON_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BLUEMAGICCOMMON_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef BLUEMAGICCOMMON_EXPORTS
#define BLUEMAGICCOMMON_API __declspec(dllexport)
#else
#define BLUEMAGICCOMMON_API __declspec(dllimport)
#endif

// This class is exported from the BlueMagicCommon.dll
class BLUEMAGICCOMMON_API CBlueMagicCommon {
public:
	CBlueMagicCommon(void);
	// TODO: add your methods here.
};

extern BLUEMAGICCOMMON_API int nBlueMagicCommon;

BLUEMAGICCOMMON_API int fnBlueMagicCommon(void);
