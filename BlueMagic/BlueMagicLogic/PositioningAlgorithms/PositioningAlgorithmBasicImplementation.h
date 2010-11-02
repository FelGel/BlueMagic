#pragma once

#include <map>
#include "PositioningAlgorithms.h"
#include "IPositioningAlgorithmImplementation.h"

#include "RssiToDistanceBasicAlgorithmManager.h"
#include "DistanceSmoothingBasicAlgorithmGeneralManager.h"
#include "PositioningBasicAlgorithmManager.h"

class POSITIONINGALGORITHMS_API CPositioningAlgorithmBasicImplementation :
	public IPositioningAlgorithmImplementation
{
public:
	CPositioningAlgorithmBasicImplementation(void);
	~CPositioningAlgorithmBasicImplementation(void);

	virtual bool Init();
	virtual SPosition CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts);

private:
	void DumpPositioningParamsToLog(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts);
	bool ReadSensorsConfiguration(
		std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams> &SensorsDistanceParams,
		std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams> &SensorsSmoothingParams,
		std::map<int /*SensorID*/, SPosition> &SensorsLocationParams);
	void EstimateDistances(
		std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts,
		std::map<int /*SensorID*/, double /*Distance*/> &DistanceEstimations);

private:
	CRssiToDistanceBasicAlgorithmManager			m_DistanceAlgorithm;
	CDistanceSmoothingBasicAlgorithmGeneralManager	m_SmoothingAlgorithm;
	CPositioningBasicAlgorithmManager				m_PositioningAlgorithm;
};
