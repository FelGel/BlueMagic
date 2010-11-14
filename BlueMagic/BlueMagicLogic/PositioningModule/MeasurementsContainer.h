#pragma once

#include <map>
#include "PositioningAlgorithms/PositionStructure.h"

class CMeasurementsContainer
{
public:
	CMeasurementsContainer(void);
	~CMeasurementsContainer(void);

	bool AddMeasurement(DWORD TickCount, int RSSI);
	bool GetClosestMeasurement(const DWORD TickCountRefference, SMeasurement &Measurement, DWORD MaxTickCountDistance) const;
	void ClearMeasurementsOlderThan(DWORD TickCount);
	bool GetTickCountOfLastMeasurement(DWORD &LastTickCount);

private:
	std::map<DWORD /*Tick Count*/, int /*RSSI*/> m_Measurements;
};
