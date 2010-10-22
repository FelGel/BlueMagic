// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the POSITIONINGALGORITHMS_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// POSITIONINGALGORITHMS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef POSITIONINGALGORITHMS_EXPORTS
	//#ifndef POSITIONINGALGORITHMS_API
	//#pragma message("Positioning EXPORT\n")
	//#endif
#define POSITIONINGALGORITHMS_API __declspec(dllexport)
#else
	//#ifndef POSITIONINGALGORITHMS_API
	//#pragma message("Positioning IMPORT\n")
	//#endif
#define POSITIONINGALGORITHMS_API __declspec(dllimport)
#endif

// This class is exported from the PositioningAlgorithms.dll
// class POSITIONINGALGORITHMS_API CPositioningAlgorithms {
// public:
// 	CPositioningAlgorithms(void);
// 	// TODO: add your methods here.
// };
// 
// extern POSITIONINGALGORITHMS_API int nPositioningAlgorithms;
// 
// POSITIONINGALGORITHMS_API int fnPositioningAlgorithms(void);
