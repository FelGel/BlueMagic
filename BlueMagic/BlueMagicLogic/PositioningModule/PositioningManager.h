#pragma once

#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"
#include "SensorControllersContainer.h"
#include "EstablishmentTopology.h"
#include "PositioningAlgorithm.h"

class CPositioningManager :	public CThreadWithQueue, public ISensorEvents, public IPositioningEvents
{
public:
	CPositioningManager(void);
	~CPositioningManager(void);

	// SensorEvents
	virtual void OnErrorInTopology(UCHAR SensorId);
	virtual void OnSensorInfo(SSensorInfo SensorInfo);
	virtual void OnSensorsInfo(CList<SSensorInfo> *SensorsInfo);
	virtual void OnIncomingScannedData(CList<SScannedData> *ScannedDataList);
	virtual void OnConnected(UCHAR SensorId);
	virtual void OnDisconnected(UCHAR SensorId);

	// Positioning Algorithm Events
	virtual void OnPositioning(std::string BDADDRESS, SPosition Position, double Accuracy, DWORD TimeStamp, int StoreID, bool IsInStore);

	// General Public functions
	void Advise(IPositioningInterface *PositioningInterfaceHandler) {m_PositioningInterfaceHandler = PositioningInterfaceHandler;}
	bool Init();
	virtual void OnThreadClose();

private:
	void HandleDataReceived(const SScannedData& ScannedData);

private:
	CSensorControllersContainer m_SensorControllersContainer;
	CEstablishmentTopology m_EstablishmentTopology;//CEstablishmentTopologysContainer m_Establishments; 
	CPositioningAlgorithm m_PositioningAlgorithm;
	IPositioningInterface *m_PositioningInterfaceHandler;
};
