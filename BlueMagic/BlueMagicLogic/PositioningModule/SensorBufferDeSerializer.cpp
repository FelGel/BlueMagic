#include "StdAfx.h"
#include "SensorBufferDeSerializer.h"

CSensorBufferDeSerializer::CSensorBufferDeSerializer(CSensorController *SensorController, const char* ContextStr, const BYTE* DataOrigin, int DataSize, int RelevantOffset /*= 0*/, bool AllowOverflowAttempt /*= false*/)
	: CBufferDeSerializer(ContextStr, DataOrigin, DataSize, RelevantOffset, AllowOverflowAttempt), m_SensorController(SensorController)
{
}

CSensorBufferDeSerializer::~CSensorBufferDeSerializer(void)
{
}
