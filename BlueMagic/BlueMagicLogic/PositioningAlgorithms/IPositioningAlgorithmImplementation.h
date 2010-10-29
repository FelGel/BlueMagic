#pragma once
#include "PositioningAlgorithms.h"
#include "PositionStructure.h"

#include <map>

class POSITIONINGALGORITHMS_API IPositioningAlgorithmImplementation
{
public:
	virtual SPosition CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts) = 0;
};