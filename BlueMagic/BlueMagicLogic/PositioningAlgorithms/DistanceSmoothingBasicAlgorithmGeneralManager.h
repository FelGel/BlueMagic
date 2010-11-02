#pragma once

#include <map>
#include "PositioningAlgorithms.h"
#include "DistanceSmoothingBasicAlgorithmManager.h"

// The General Manager is responsible for all BdAddresses and all SensorIDs

struct SDistanceSmoothingAlgorithmParams
{
	SDistanceSmoothingAlgorithmParams(double _a, double _b) : a(_a), b(_b) {}

	double a;
	double b;
};

class POSITIONINGALGORITHMS_API CDistanceSmoothingBasicAlgorithmGeneralManager
{
public:
	CDistanceSmoothingBasicAlgorithmGeneralManager(void);
	~CDistanceSmoothingBasicAlgorithmGeneralManager(void);

	bool Init(std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams> SensorsParams);

	double SmoothDistance(int SensorID, std::string BDADDRESS, double Rcurrent, double Tcurrent /*Current TickCount of measurement*/);

	double Geta(int SensorID);
	double Getb(int SensorID);

	double GetEstimatedVelocity(int SensorID, std::string BDADDRESS);
	double GetLastTS(int SensorID, std::string BDADDRESS);

	std::map<int /*SensorID*/, CDistanceSmoothingBasicAlgorithmManager> m_SensorsAlgorithms;
};
