#pragma once

#include <vector>
#include "SensorInformation.h"

#define BDADDRESS_LENGTH_IN_BYTES 6

struct SSensorInfo
{
	/*short*/ int Clock;
	int Version;
	std::string SensorBDADDRESS;
};

struct SScannedData
{
	DWORD Time; ///*short*/ int Clock;
	/*BYTE*/  int RSSI;
	//BYTE  ScannedBDADDRESS[BDADDRESS_LENGTH_IN_BYTES];
	std::string ScannedBDADDRESS;
};

class ISensorEvents
{
public:
	virtual void OnErrorInTopology(UCHAR SensorId) = 0;
	virtual void OnSensorInfo(int SensorId, SSensorInfo SensorInfo) = 0;
	virtual void OnIncomingScannedData(int SensorId, SScannedData ScannedData) = 0;
	virtual void OnSensorInSystem(int SensorId, bool IsController, std::string BDADDRESS, std::vector<int> ChildrenSensorIDs) = 0;
	virtual void OnSensorStatusUpdate(int SensorId, bool IsController, ESensorConnectionStatus SensorConnectionStatus, ESensorHandshakeStatus SensorHandshakeStatus, ESensorActivityStatus SensorActivityStatus) = 0;
};
