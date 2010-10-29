#include "StdAfx.h"
#include "PositioningAlgorithmBasicImplementation.h"
#include "Common/LogEvent.h"

CPositioningAlgorithmBasicImplementation::CPositioningAlgorithmBasicImplementation(void)
{
}

CPositioningAlgorithmBasicImplementation::~CPositioningAlgorithmBasicImplementation(void)
{
}

/*virtual*/ SPosition CPositioningAlgorithmBasicImplementation::CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts)
{
	// ToDo: Move this to DumpPositionData, and do actual implementation

	std::string PositionDataString;

	std::map<int /*SensorID*/, SMeasurement>::iterator Iter = Measuremnts.begin();
	std::map<int /*SensorID*/, SMeasurement>::iterator End = Measuremnts.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		SMeasurement &Measurement = Iter->second;

		CString MeasurementString;
		MeasurementString.Format("ID: %d, RSSI: %d. ", SensorID, Measurement.m_RSSI);

		PositionDataString += MeasurementString;
	}

	LogEvent(LE_NOTICE, __FUNCTION__ ": Positioning Parameters for BDADDRESS %s. Data: %s", 
		BDADDRESS.c_str(), PositionDataString.c_str());

	return SPosition();
}