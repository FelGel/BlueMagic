#pragma once

#include <list>

struct SSensorInfo
{
	UCHAR SensorId;
	short Clock;
	int Version;
};

struct SScannedData
{
	UCHAR SensorId;
	short Clock;
	BYTE  RSSI;
	BYTE  ScannedBDADDRESS[6];
};

class ISensorEvents
{
	virtual void OnErrorInTopolog(UCHAR SensorId) = 0;
	virtual void OnSensorInfo(SSensorInfo SensorInfo) = 0;
	virtual void OnSensorsInfo(CList<SSensorInfo> SensorsInfo) = 0;
	virtual void OnIncomingScannedData(CList<SScannedData> ScannedData) = 0;
	virtual void OnConnected(UCHAR SensorId) = 0;
	virtual void OnDisconnected(UCHAR SensorId) = 0;
};
