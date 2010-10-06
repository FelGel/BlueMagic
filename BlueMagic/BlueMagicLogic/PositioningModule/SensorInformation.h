#pragma once
#include <list>

#define DATA_BUFFER_SIZE	10 * 1024 // 10kb
#define MAX_SENSOR_CLOCK_VALUE				SHRT_MAX
#define MIN_SENSOR_CLOCK_VALUE				SHRT_MIN

struct SClockCorrelationData
{
	SClockCorrelationData()
	{
		SensorClock = MAX_SENSOR_CLOCK_VALUE + 1;
		MatchingTickCount = 0;
	}

	int SensorClock;
	DWORD MatchingTickCount;
};

enum ESensorConnectionStatus
{
	SensorNotConnected,
	SensorConnected,
	SensorAttemptsAtConnection,
	SensorResettingConnection,
};

enum ESensorHandshakeStatus
{
	SensorNotHandshaked,
	SensorHandshaked,
	SensorHandshakeFailed
};

enum ESensorActivityStatus
{
	SensorNotActive,
	SensorActive
};

struct SSensorInformation
{
	SSensorInformation(std::string BDADDRESS, int SensorID, std::vector<int> ChildrenSensorIDs) :
	m_BDADDRESS(BDADDRESS), m_SensorID(SensorID), 
		m_LogicalConnectionStatus(SensorNotActive), m_LastPacketRecievedTickCount(0),
		m_HandshakeStatus(SensorNotHandshaked) /*, m_LastHandshakeAttemptTickCount(0)*/
	{
		m_ChildrenSensorIDs = ChildrenSensorIDs;
	};

	std::string m_BDADDRESS;
	int m_SensorID;

	SClockCorrelationData m_ClockCorrelationData;

	ESensorActivityStatus m_LogicalConnectionStatus;
	DWORD m_LastPacketRecievedTickCount;

	ESensorHandshakeStatus m_HandshakeStatus;
	// DWORD m_LastHandshakeAttemptTickCount; - done as a Broadcast !!

	std::vector<int> m_ChildrenSensorIDs;
};
