#include "StdAfx.h"
#include "ScannedBdAdressesDataBase.h"
#include "Common/collectionhelper.h"
#include "Common/LogEvent.h"


#define GET_ScannedBdAdressData(ScannedBDADDRESS, ScannedBdAdressData)\
if (!GetValueInMap(m_ScannedDataBase, ScannedBDADDRESS, ScannedBdAdressData, false))								\
{																												\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Get ScannedBdAdressData for %s", ScannedBDADDRESS.c_str());	\
	return false;																								\
}

#define GET_ScannedBdAdressData_RetVal(ScannedBDADDRESS, ScannedBdAdressData, RetVal)\
if (!GetValueInMap(m_ScannedDataBase, ScannedBDADDRESS, ScannedBdAdressData, false))								\
{																												\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Get ScannedBdAdressData for %s", ScannedBDADDRESS.c_str());	\
	return RetVal;																								\
}

CScannedBdAdressesDataBase::CScannedBdAdressesDataBase(void)
{
}

CScannedBdAdressesDataBase::~CScannedBdAdressesDataBase(void)
{
}


bool CScannedBdAdressesDataBase::NewData(std::string ScannedBDADDRESS, int SensorID, int RSSI, DWORD ScanTickCount)
{
	SScannedBdAdressData *ScannedBdAdressData;
	if (!GetValueInMap(m_ScannedDataBase, ScannedBDADDRESS, ScannedBdAdressData, true))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Get\\Create ScannedBdAdressData for %s", ScannedBDADDRESS.c_str());
		return false;
	}

	ScannedBdAdressData->m_LastDataReceivedTickCount = ScanTickCount;

	CMeasurementsContainer *MeasurementsContainer;
	if (!GetValueInMap(ScannedBdAdressData->m_ScannedData, SensorID, MeasurementsContainer, true))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Get\\Create ScannedBdAdressLastSensorData for %s & SensorID %d", 
			ScannedBDADDRESS.c_str(), SensorID);
		return false;
	}

	if (!MeasurementsContainer->AddMeasurement(ScanTickCount, RSSI))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add measurement for %s & SensorID %d", 
			ScannedBDADDRESS.c_str(), SensorID);
		return false;
	}

	return true;
}


bool CScannedBdAdressesDataBase::BdAdressPositionUpdated(std::string ScannedBDADDRESS)
{
	SScannedBdAdressData *ScannedBdAdressData;
	GET_ScannedBdAdressData(ScannedBDADDRESS, ScannedBdAdressData);

	ScannedBdAdressData->m_LastPositioningUpdateTickCount = GetTickCount();

	return true;
}


void CScannedBdAdressesDataBase::CleanDataBase(DWORD MaxTicksWithoutData)
{
	std::map<std::string /*ScannedBDADDRESS*/, SScannedBdAdressData>::iterator Iter = m_ScannedDataBase.begin();
	std::map<std::string /*ScannedBDADDRESS*/, SScannedBdAdressData>::iterator End = m_ScannedDataBase.end();

	DWORD Now = GetTickCount();

	std::vector<std::string> RemoveList;

	for(;Iter != End; ++Iter)
	{
		SScannedBdAdressData &ScannedBdAdressData = Iter->second;
		
		if (Now - ScannedBdAdressData.m_LastDataReceivedTickCount > MaxTicksWithoutData)
			RemoveList.push_back(Iter->first);
	}

	for (unsigned int i = 0; i < RemoveList.size(); i++)
	{
		LogEvent(LE_INFO, __FUNCTION__ ": %s removed from DataBase due to Timeout", RemoveList[i].c_str());
		RemoveValueFromMap(m_ScannedDataBase, RemoveList[i]);
	}
}


std::vector<std::string> CScannedBdAdressesDataBase::GetBdAddressesToUpdate(DWORD MaxTicksWithoutPositioningUpdate)
{
	std::vector<std::string> BdAddressesToUpdate;

	std::map<std::string /*ScannedBDADDRESS*/, SScannedBdAdressData>::iterator Iter = m_ScannedDataBase.begin();
	std::map<std::string /*ScannedBDADDRESS*/, SScannedBdAdressData>::iterator End = m_ScannedDataBase.end();

	DWORD Now = GetTickCount();

	for(;Iter != End; ++Iter)
	{
		SScannedBdAdressData &ScannedBdAdressData = Iter->second;

		if (Now - ScannedBdAdressData.m_LastPositioningUpdateTickCount > MaxTicksWithoutPositioningUpdate)
			BdAddressesToUpdate.push_back(Iter->first);
	}

	return BdAddressesToUpdate;
}

std::map<int /*SensorID*/, SMeasurement> CScannedBdAdressesDataBase::GetScannedData(std::string ScannedBDADDRESS, DWORD LastDataTickCount, DWORD MaxTickCountDistance)
{
	std::map<int /*SensorID*/, SMeasurement> Measurements;

	SScannedBdAdressData *ScannedBdAdressData;
	GET_ScannedBdAdressData_RetVal(ScannedBDADDRESS, ScannedBdAdressData, Measurements);

	std::map<int /*SensorID*/, CMeasurementsContainer>::iterator Iter = ScannedBdAdressData->m_ScannedData.begin();
	std::map<int /*SensorID*/, CMeasurementsContainer>::iterator End = ScannedBdAdressData->m_ScannedData.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		CMeasurementsContainer &MeasurementsContainer = Iter->second;

		SMeasurement Measurement;
		if (MeasurementsContainer.GetClosestMeasurement(LastDataTickCount, Measurement, MaxTickCountDistance))
		{
			if (!InsertValueToMap(Measurements, SensorID, Measurement))
			{
				LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add measurement to list.");
			}
		}	
	}
	
	return Measurements;
}

// Function gets last tick count of for every sensor.
// Then, it takes the oldest of them, as long as it is not too old (i.e. not older than MaxTickCountDistanceFromNewest from newest)
DWORD CScannedBdAdressesDataBase::GetOldestLastTickCount(std::string ScannedBDADDRESS, DWORD MaxTickCountDistanceFromNewest)
{
	std::vector<DWORD> LastTickCountList = GetLastTickCountForAllSensors(ScannedBDADDRESS);

	// Find Max
	DWORD MaxLastTickCount = 0;
	for (unsigned int i = 0; i < LastTickCountList.size(); i++)
		MaxLastTickCount = max(MaxLastTickCount, LastTickCountList[i]);

	// Find Min which is not too far from max
	DWORD OldestLastTickCount = MaxLastTickCount;
	for (unsigned int i = 0; i < LastTickCountList.size(); i++)
	{
		if (LastTickCountList[i] < OldestLastTickCount &&
			MaxLastTickCount - LastTickCountList[i] < MaxTickCountDistanceFromNewest)
		{
			OldestLastTickCount = LastTickCountList[i];
		}
	}

	return OldestLastTickCount;
}

std::vector<DWORD> CScannedBdAdressesDataBase::GetLastTickCountForAllSensors(std::string ScannedBDADDRESS)
{
	std::vector<DWORD> LastTickCountList;

	SScannedBdAdressData *ScannedBdAdressData;
	GET_ScannedBdAdressData_RetVal(ScannedBDADDRESS, ScannedBdAdressData, LastTickCountList);

	std::map<int /*SensorID*/, CMeasurementsContainer>::iterator Iter = ScannedBdAdressData->m_ScannedData.begin();
	std::map<int /*SensorID*/, CMeasurementsContainer>::iterator End = ScannedBdAdressData->m_ScannedData.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		CMeasurementsContainer &MeasurementsContainer = Iter->second;

		DWORD LastTickCount = 0;
		if (MeasurementsContainer.GetTickCountOfLastMeasurement(LastTickCount))
			LastTickCountList.push_back(LastTickCount);
	}

	return LastTickCountList;
}