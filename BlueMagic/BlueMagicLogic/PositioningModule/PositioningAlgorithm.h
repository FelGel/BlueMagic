#pragma once

#include "EstablishmentTopology.h"
#include "IPositioningInterface.h"
#include "ISensorEvents.h"
#include "ScannedBdAdressesDataBase.h"
#include "PositioningAlgorithms/IPositioningAlgorithmImplementation.h"

// Note: Class is running withing PositioningManager's worker thread

class IPositioningEvents
{
public:
	virtual void OnPositioning(std::string BDADDRESS, SPosition Position, double Accuracy, DWORD TimeStamp, int StoreID, bool IsInStore) = 0;
};

class CPositioningAlgorithm
{
public:
	CPositioningAlgorithm(void);
	~CPositioningAlgorithm(void);

	void Init();
	void Advise(CEstablishmentTopology* EstablishmentTopology, IPositioningEvents *PositioningEventsHandler);
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
};
