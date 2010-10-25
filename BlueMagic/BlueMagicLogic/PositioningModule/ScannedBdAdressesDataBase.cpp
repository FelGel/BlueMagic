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

	CMeasurementsContainer *Measurements;
	if (!GetValueInMap(ScannedBdAdressData->m_ScannedData, SensorID, Measurements, true))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Get\\Create ScannedBdAdressLastSensorData for %s & SensorID %d", 
			ScannedBDADDRESS.c_str(), SensorID);
		return false;
	}

	if (!Measurements->AddMeasurement(ScanTickCount, RSSI))
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

	return BdAddressesToUpdate;
}

std::map<int /*SensorID*/, SMeasurement> CScannedBdAdressesDataBase::GetScannedData(std::string ScannedBDADDRESS)
{
	std::map<int /*SensorID*/, SMeasurement> K;
	return K;
}


std::map<int /*SensorID*/, SMeasurement> CScannedBdAdressesDataBase::GetScannedData(std::string ScannedBDADDRESS, DWORD MaxTickCountDistance)
{
	std::map<int /*SensorID*/, SMeasurement> K;
	return K;
}