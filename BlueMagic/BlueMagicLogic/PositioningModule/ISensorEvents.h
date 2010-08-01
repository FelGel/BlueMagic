#pragma once

#include <list>
#define BDADDRESS_LENGTH_IN_BYTES 6

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
	BYTE  ScannedBDADDRESS[BDADDRESS_LENGTH_IN_BYTES];
};

class ISensorEvents
{
public:
	virtual void OnErrorInTopology(UCHAR SensorId) = 0;
	virtual void OnSensorInfo(SSensorInfo SensorInfo) = 0;
	virtual void OnSensorsInfo(CList<SSensorInfo> *SensorsInfo) = 0;
	virtual void OnIncomingScannedData(CList<SScannedData> *ScannedData) = 0;
	virtual void OnConnected(UCHAR SensorId) = 0;
	virtual void OnDisconnected(UCHAR SensorId) = 0;
};
