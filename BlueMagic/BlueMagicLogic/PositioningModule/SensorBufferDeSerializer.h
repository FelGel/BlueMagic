#pragma once
#include <vector>
#include "Common\DeSerializer.h"
#include "Common\bufferdeserializer.h"

class CSensorController;

class CSensorBufferDeSerializer : public CBufferDeSerializer
{
public:
	CSensorBufferDeSerializer(CSensorController *SensorController, const char* ContextStr, const BYTE* DataOrigin, int DataSize, int RelevantOffset = 0, bool AllowOverflowAttempt = false);
	~CSensorBufferDeSerializer(void);

	virtual bool GetNextTimetField(time_t& /*DataToGet*/) {return false;} /*due to an unexplained not required UNRESOLVED EXTERNAL SYMBOL*/

	CSensorController *GetSensorController() {return m_SensorController;}

private:
	CSensorController *m_SensorController;
};
