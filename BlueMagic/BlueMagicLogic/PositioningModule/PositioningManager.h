#pragma once

#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"
#include "SensorControllersContainer.h"

class CPositioningManager :	public CThreadWithQueue, public ISensorEvents
{
public:
	CPositioningManager(void);
	~CPositioningManager(void);

	virtual void OnErrorInTopology(UCHAR SensorId);
	virtual void OnSensorInfo(SSensorInfo SensorInfo);
	virtual void OnSensorsInfo(CList<SSensorInfo> *SensorsInfo);
	virtual void OnIncomingScannedData(CList<SScannedData> *ScannedData);
	virtual void OnConnected(UCHAR SensorId);
	virtual void OnDisconnected(UCHAR SensorId);

	bool Init();
	virtual void OnThreadClose();

private:
	CSensorControllersContainer m_SensorControllersContainer;
};
