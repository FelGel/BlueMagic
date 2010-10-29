#pragma once
#include "IPositioningAlgorithmImplementation.h"

class POSITIONINGALGORITHMS_API CPositioningAlgorithmBasicImplementation :
	public IPositioningAlgorithmImplementation
{
public:
	CPositioningAlgorithmBasicImplementation(void);
	~CPositioningAlgorithmBasicImplementation(void);

	virtual SPosition CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts);
};
