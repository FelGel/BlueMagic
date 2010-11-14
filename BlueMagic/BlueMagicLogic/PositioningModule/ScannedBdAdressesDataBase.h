#pragma once

#include <map>
#include <vector>
#include "MeasurementsContainer.h"

struct SScannedBdAdressData
{
	SScannedBdAdressData() : m_LastDataReceivedTickCount(0), m_LastPositioningUpdateTickCount(0) {}

	DWORD m_LastDataReceivedTickCount; /*So entry will be deleted after timeout*/
	DWORD m_LastPositioningUpdateTickCount; /*So positioning will take place at least every certain timeout*/
	std::map<int /*SensorID*/, CMeasurementsContainer> m_ScannedData;
};


class CScannedBdAdressesDataBase
{
public:
	CScannedBdAdressesDataBase(void);
	~CScannedBdAdressesDataBase(void);

public:
	bool NewData(std::string ScannedBDADDRESS, int SensorID, int RSSI, DWORD ScanTickCount);
	bool BdAdressPositionUpdated(std::string ScannedBDADDRESS);
	void CleanDataBase(DWORD MaxTicksWithoutData);
	std::vector<std::string> GetBdAddressesToUpdate(DWORD MaxTicksWithoutPositioningUpdate);	
	DWORD GetOldestLastTickCount(std::string ScannedBDADDRESS, DWORD MaxTickCountDistanceFromNewest);
	std::map<int /*SensorID*/, SMeasurement> GetScannedData(std::string ScannedBDADDRESS, DWORD LastDataTickCount, DWORD MaxTickCountDistance);
	//std::map<int /*SensorID*/, SMeasurement> GetScannedData(std::string ScannedBDADDRESS);
	//std::map<int /*SensorID*/, SMeasurement> GetScannedData(std::string ScannedBDADDRESS, DWORD MaxTickCountDistance, DWORD MinNumberOfParticipatingSensor);
	//bool HasEnoughNewDataGatheredSinceLastUpdate(std::string ScannedBDADDRESS, DWORD MinNumberOfParticipatingSensor);

private:
	std::vector<DWORD> GetLastTickCountForAllSensors(std::string ScannedBDADDRESS);

private:
	std::map<std::string /*ScannedBDADDRESS*/, SScannedBdAdressData> m_ScannedDataBase;
};
