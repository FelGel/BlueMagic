#pragma once

#include "EstablishmentTopology.h"
#include "IPositioningInterface.h"
#include "ISensorEvents.h"

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

	void Advise(CEstablishmentTopology* EstablishmentTopology, IPositioningEvents *PositioningEventsHandler);

	void OnScannedData(const SScannedData& ScannedData);

private:
	CEstablishmentTopology* m_EstablishmentTopology;
	IPositioningEvents *m_PositioningEventsHandler;
};
