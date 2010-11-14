#pragma once

#include <map>
#include "PositioningAlgorithms.h"
#include "RssiToDistanceBasicAlgorithm.h"

struct SRssiToDistanceBasicAlgorithmParams
{
	SRssiToDistanceBasicAlgorithmParams(double _A, double _N) : A(_A), N(_N) {}
	double A;
	double N;
};

class POSITIONINGALGORITHMS_API CRssiToDistanceBasicAlgorithmManager
{
public:
	CRssiToDistanceBasicAlgorithmManager(void);
	~CRssiToDistanceBasicAlgorithmManager(void);

	bool Init(std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams> SensorsParams);

	double CalcDistance(int SensorID, int RSSI);

	double GetA(int SensorID);
	double GetN(int SensorID);

	std::map<int /*SensorID*/, CRssiToDistanceBasicAlgorithm> m_SensorsAlgorithms;
};
