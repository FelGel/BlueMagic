#pragma once

#include "PositioningAlgorithms.h"

#define ILLEGAL_A +999999
#define ILLEGAL_N +999999

class POSITIONINGALGORITHMS_API CRssiToDistanceBasicAlgorithm
{
public:
	CRssiToDistanceBasicAlgorithm(void);
	CRssiToDistanceBasicAlgorithm(double A, double N);
	~CRssiToDistanceBasicAlgorithm(void);

	void Init(double A, double N);

	double CalcDistance(int RSSI) const;
	static double CalcDistance(int RSSI, double A, double N);

	double GetA() const {return m_A;}
	double GetN() const {return m_N;}

private:
	double m_A;
	double m_N;
};
