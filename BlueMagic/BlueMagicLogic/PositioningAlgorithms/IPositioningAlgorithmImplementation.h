#pragma once
#include "PositioningAlgorithms.h"
#include "PositionStructure.h"

#include <map>

class CEstablishmentTopology;

class IPositioningDebugReport
{
public:
	virtual void OnPositioningDebugReport(
		std::string BDADDRESS, 
		std::map<int /*SensorID*/, SMeasurement> Measurements,
		std::map<int /*SensorID*/, double /*SmoothedDistance*/> DistanceEstimations,
		SPosition EstimatedPosition,
		SPosition EstimatedPositionError,
		int NumOfIterations,
		bool IsInEstablishment) = 0;

	virtual void OnSensorsLocationReport(
		std::map<int /*SensorID*/, SPosition> SensorsLocation) = 0;
};


class POSITIONINGALGORITHMS_API IPositioningAlgorithmImplementation
{
public:
	virtual SPosition CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts, SPosition &Accuracy, bool &IsInEstablishment) = 0;
	virtual bool Init() = 0;
	virtual void AdviseDebugReport(IPositioningDebugReport *DebugReport) = 0;
	virtual void AdviseEstablishmentTopology(CEstablishmentTopology *EstablishmentTopology) = 0;
};