#include "StdAfx.h"
#include "SensorInformation.h"

std::string SSensorInformation::SensorConnectionStatusToShortString(ESensorConnectionStatus SensorConnectionStatus)
{
	switch(SensorConnectionStatus)
	{
		RETURN_TYPE_STR1(Sensor,NotConnected);
		RETURN_TYPE_STR1(Sensor,Connected);
		RETURN_TYPE_STR2(Sensor,Attempts,AtConnection);
		RETURN_TYPE_STR2(Sensor,Resetting,Connection);

	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown %d", SensorConnectionStatus);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}


std::string SSensorInformation::SensorHandshakeStatusToShortString(ESensorHandshakeStatus SensorHandshakeStatus)
{
	switch(SensorHandshakeStatus)
	{
		RETURN_TYPE_STR1(Sensor,NotHandshaked);
		RETURN_TYPE_STR1(Sensor,Handshaked);
		RETURN_TYPE_STR1(Sensor,HandshakeFailed);

	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown %d", SensorHandshakeStatus);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}

std::string SSensorInformation::SensorActivityStatusToShortString(ESensorActivityStatus SensorActivityStatus)
{
	switch(SensorActivityStatus)
	{
		RETURN_TYPE_STR1(Sensor,NotActive);
		RETURN_TYPE_STR1(Sensor,Active);

	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown %d", SensorActivityStatus);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}
