#pragma once

#include "IPositioningInterface.h"
#include "ISensorEvents.h"
#include "ScannedBdAdressesDataBase.h"
#include "PositioningAlgorithms/EstablishmentTopology.h"
#include "PositioningAlgorithms/IPositioningAlgorithmImplementation.h"

// Note: Class is running withing PositioningManager's worker thread

class IPositioningEvents
{
public:
	virtual void OnPositioning(std::string BDADDRESS, SPosition Position, double Accuracy, DWORD TimeStamp, int StoreID, bool IsInStore, std::vector<std::string> DepartmentNamesUserCurrentlyIn) = 0;
};

class CPositioningAlgorithm
{
public:
	CPositioningAlgorithm(void);
	~CPositioningAlgorithm(void);

	bool Init();
	void Advise(
		CEstablishmentTopology* EstablishmentTopology, 
		IPositioningEvents *PositioningEventsHandler,
		IPositioningDebugReport *PositioningDebugReportHandler);
	void Close();

	void OnScannedData(const int &SensorId, const SScannedData& ScannedData);
	void OnTimeout();

private:
	void DoPositioning(std::string BdAddress, DWORD LastDataTickCount, DWORD NumberOfParticipatingSensor, DWORD TickCountDifferenceBetweenMeasuremnts);
	void PositionOutOfDateBdAddresses();

private:
	CEstablishmentTopology* m_EstablishmentTopology;
	IPositioningEvents *m_PositioningEventsHandler;
	IPositioningAlgorithmImplementation *m_Impl;

	CScannedBdAdressesDataBase m_ScannedBdAdressesDataBase;
	DWORD m_MinNumberOfParticipatingSensor;
	DWORD m_DesiredNumberOfParticipatingSensor;
	DWORD m_MaxTimeBetweenUpdates;
	DWORD m_DesiredTickCountDifferenceBetweenMeasuremnts;
	DWORD m_MaxTickCountDifferenceBetweenMeasuremnts;
	DWORD m_TimeoutForRemovingBdaddress;
	DWORD m_CleaningTimeoutResolution;
	DWORD m_UpdateTimeoutResolution;
	DWORD m_LastCleaningTimeoutTickCount;
	DWORD m_LastUpdateTimeoutTickCount;
};
