#pragma once

#include "PositioningAlgorithms.h"

#define ILLEGAL_a +999999
#define ILLEGAL_b +999999

class POSITIONINGALGORITHMS_API CDistanceSmoothingBasicAlgorithm
{
public:
	CDistanceSmoothingBasicAlgorithm(void);
	CDistanceSmoothingBasicAlgorithm(double a, double b);
	~CDistanceSmoothingBasicAlgorithm(void);

	void Init(double a, double b);

	double SmoothDistance(double Rcurrent, double Tcurrent /*Current TickCount of measurement*/);

	double Geta() const {return m_a;}
	double Getb() const {return m_b;}

	double GetEstimatedVelocity() const {return m_Vpred;}
	double GetLastTS() const {return m_LastTS;}

private:
	double CalcTS(double CurrentTickCount);

private:
	double m_a;
	double m_b;

	double m_Rpred;
	double m_Vpred;
	double m_Tpred;

	double m_LastTS;
};
