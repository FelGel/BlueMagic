#pragma once

#include <map>

#define IRRELEVANT_RSSI -1000

struct SMeasurement
{
	SMeasurement() : m_RSSI(IRRELEVANT_RSSI), m_TickCount(0) {}

	int		m_RSSI;
	DWORD	m_TickCount;
};


class CMeasurementsContainer
{
public:
	CMeasurementsContainer(void);
	~CMeasurementsContainer(void);

	bool AddMeasurement(DWORD TickCount, int RSSI);
	bool GetClosestMeasurement(const DWORD TickCountRefference, SMeasurement &Measurement, DWORD MaxTickCountDistance) const;
	void ClearMeasurementsOlderThan(DWORD TickCount);

private:
	std::map<DWORD /*Tick Count*/, int /*RSSI*/> m_Measurements;
};
