#include "StdAfx.h"
#include "SensorBufferDeSerializer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSensorBufferDeSerializer::CSensorBufferDeSerializer(CSensorController *SensorController, const char* ContextStr, const BYTE* DataOrigin, int DataSize, int RelevantOffset /*= 0*/, bool AllowOverflowAttempt /*= false*/)
	: CBufferDeSerializer(ContextStr, DataOrigin, DataSize, RelevantOffset, AllowOverflowAttempt), m_SensorController(SensorController)
{
}

CSensorBufferDeSerializer::~CSensorBufferDeSerializer(void)
{
}
