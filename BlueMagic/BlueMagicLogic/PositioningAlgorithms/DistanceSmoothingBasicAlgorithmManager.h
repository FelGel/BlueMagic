#pragma once

#include <map>
#include "DistanceSmoothingBasicAlgorithm.h"
#include "PositioningAlgorithms.h"
#include "PositionStructure.h"

// Responsible for all BdAddresses of same SensorID
// The General Manager is responsible for all BdAddresses and all SensorIDs

class POSITIONINGALGORITHMS_API CDistanceSmoothingBasicAlgorithmManager
{
public:
	CDistanceSmoothingBasicAlgorithmManager(void);
	CDistanceSmoothingBasicAlgorithmManager(double a, double b);
	~CDistanceSmoothingBasicAlgorithmManager(void);

	void Init(double a, double b);

	double SmoothDistance(std::string BDADDRESS, double Rcurrent, double Tcurrent /*Current TickCount of measurement*/);

	double Geta() const {return m_a;}
	double Getb() const {return m_b;}

	double GetEstimatedVelocity(std::string BDADDRESS);
	double GetLastTS(std::string BDADDRESS);

private:
	double m_a;
	double m_b;

	std::map<std::string /*BDADDRESS*/, CDistanceSmoothingBasicAlgorithm> m_DetectedBDaddresses;
};
