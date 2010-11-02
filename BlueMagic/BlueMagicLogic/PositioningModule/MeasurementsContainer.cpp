#include "StdAfx.h"
#include "MeasurementsContainer.h"

#include "Common/LogEvent.h"
#include "Common/collectionhelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CMeasurementsContainer::CMeasurementsContainer(void){}
CMeasurementsContainer::~CMeasurementsContainer(void){}


bool CMeasurementsContainer::AddMeasurement(DWORD TickCount, int RSSI)
{
	// It is possible to receive two measurement in the same TickCount !!
	// in such a case, the highest of them will be taken.
	if (IsValueInMap(m_Measurements, TickCount))
	{
		int *RssiInMap;
		if (!GetValueInMap(m_Measurements, TickCount, RssiInMap))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to get value from map !");
			return false;
		}

		LogEvent(LE_INFO, __FUNCTION__ ": Two Measurements received on same time: %d & %d. Taking the higher value.",
			*RssiInMap, RSSI);

		*RssiInMap = max(*RssiInMap, RSSI);
		
		return true;
	}

	if (!InsertValueToMap(m_Measurements, TickCount, RSSI))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add Measurement.");
		return false;
	}

	return true;
}

bool CMeasurementsContainer::GetClosestMeasurement(DWORD TickCountRefference, SMeasurement &Measurement, DWORD MaxTickCountDistance) const
{
	// ToDo: in the future, remove all thses INFOLOW logs.
	// too much noise !

	if (GetValueFromMap(m_Measurements, TickCountRefference, Measurement.m_RSSI))
	{
		Measurement.m_TickCount = TickCountRefference;
		LogEvent(LE_INFOLOW, __FUNCTION__ ": Measurement of exact time exists in map");
		return true;
	}

	std::map<DWORD /*Tick Count*/, int /*RSSI*/>::const_iterator HigherIter 
		= m_Measurements.upper_bound(TickCountRefference);

	std::map<DWORD /*Tick Count*/, int /*RSSI*/>::const_iterator LowerIter 
		= HigherIter; LowerIter--;

	if (HigherIter == m_Measurements.end() && LowerIter == m_Measurements.end())
	{
		LogEvent(LE_INFOLOW,
			__FUNCTION__ ": No Previous measurement was found.");
		return false;
	}

	if (LowerIter == m_Measurements.end()) // Only Higher
	{
		Measurement.m_TickCount = HigherIter->first;
		Measurement.m_RSSI = HigherIter->second;
		LogEvent(LE_INFOLOW,
			__FUNCTION__ ": Higher Measurement (%d) is closest to reference (%d)",
			Measurement.m_TickCount, TickCountRefference);
	}
	else if (HigherIter == m_Measurements.end() || // Either only Lower or Lower is closer
		abs(LowerIter->first - TickCountRefference) <= abs(HigherIter->first - TickCountRefference))
	{		
		Measurement.m_TickCount = LowerIter->first;
		Measurement.m_RSSI = LowerIter->second;
		LogEvent(LE_INFOLOW,
			__FUNCTION__ ": Lower Measurement (%d) is closest to reference (%d)",
			Measurement.m_TickCount, TickCountRefference);
	}
	else if (LowerIter != m_Measurements.end()) // Higher is closer
	{
		Measurement.m_TickCount = HigherIter->first;
		Measurement.m_RSSI = HigherIter->second;
		LogEvent(LE_INFOLOW,
			__FUNCTION__ ": Higher Measurement (%d) is closest to reference (%d)",
			Measurement.m_TickCount, TickCountRefference);
	}
	else // None exists
	{
		LogEvent(LE_INFOLOW,
			__FUNCTION__ ": No Previous measurement was found..");
		return false;
	}

	if ((DWORD)abs(Measurement.m_TickCount - TickCountRefference) > MaxTickCountDistance)
	{
		LogEvent(LE_INFOLOW, __FUNCTION__ ": Closest Measurement (%d) is too far away from reference (%d).",
			Measurement.m_TickCount, TickCountRefference);
		return false;
	}

	return true;
}

void CMeasurementsContainer::ClearMeasurementsOlderThan(DWORD TickCount)
{
	std::map<DWORD /*Tick Count*/, int /*RSSI*/>::iterator Iter = m_Measurements.begin();
	std::map<DWORD /*Tick Count*/, int /*RSSI*/>::iterator End = m_Measurements.end();

	std::vector<DWORD /*Tick Count*/> RemoveList;
	for(;Iter != End; ++Iter)
	{
		if (Iter->first >= TickCount)
			break;

		RemoveList.push_back(Iter->first);
	}
	
	for (unsigned int i = 0; i < RemoveList.size(); i++)
	{
		if (!RemoveValueFromMap(m_Measurements, RemoveList[i]))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to remove value %d from Measurements map", RemoveList[i]);
		}
	}
}

bool CMeasurementsContainer::GetTickCountOfLastMeasurement(DWORD &LastTickCount)
{
	if (m_Measurements.size() == 0) // No last measurement exists
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Should never happen. container always has at least one measurement if it exists!");
		Assert(false); 
		return false; 
	}

	std::map<DWORD /*Tick Count*/, int /*RSSI*/>::iterator End = m_Measurements.end();	
	LastTickCount = (--End)->first; // return tickcount of last measurement
	return true;
}