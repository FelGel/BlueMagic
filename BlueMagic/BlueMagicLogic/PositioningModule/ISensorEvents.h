#pragma once

#include <vector>
#define BDADDRESS_LENGTH_IN_BYTES 6

struct SSensorInfo
{
	/*UCHAR*/ int SensorId;
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
	virtual void OnSensorInfo(SSensorInfo SensorInfo) = 0;
	virtual void OnSensorsInfo(CList<SSensorInfo> *SensorsInfo) = 0;
	virtual void OnIncomingScannedData(int SensorId, SScannedData ScannedData) = 0;
	virtual void OnConnected(UCHAR SensorId) = 0;
	virtual void OnDisconnected(UCHAR SensorId) = 0;
	virtual void OnSensorInSystem(int SensorId, bool IsController, std::string BDADDRESS, std::vector<int> ChildrenSensorIDs) = 0;
};
