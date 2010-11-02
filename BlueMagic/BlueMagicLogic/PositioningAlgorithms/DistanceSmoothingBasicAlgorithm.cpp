#include "StdAfx.h"
#include "DistanceSmoothingBasicAlgorithm.h"
#include "Common/LogEvent.h"

#define FIRST_MEASUREMENT -1

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CDistanceSmoothingBasicAlgorithm::CDistanceSmoothingBasicAlgorithm(void) : m_a(0), m_b(0), m_Rpred(FIRST_MEASUREMENT), m_Vpred(0), m_Tpred(0), m_LastTS(0) {}
CDistanceSmoothingBasicAlgorithm::CDistanceSmoothingBasicAlgorithm(double a, double b) : m_a(a), m_b(b), m_Rpred(FIRST_MEASUREMENT), m_Vpred(0), m_Tpred(0), m_LastTS(0) {}
CDistanceSmoothingBasicAlgorithm::~CDistanceSmoothingBasicAlgorithm(void) {}

void CDistanceSmoothingBasicAlgorithm::Init(double a, double b)
{
	m_a = a;
	m_b = b;
}

double CDistanceSmoothingBasicAlgorithm::SmoothDistance(double Rcurrent, double Tcurrent)
{
	double Rest, Vest;
	double Ts = CalcTS(Tcurrent); /*Time Since Last Measurement*/

	// Calculating Current Smoothed Distance
	if (m_Rpred == FIRST_MEASUREMENT)
	{
		Assert(m_Tpred == 0);

		// first time - no smoothing
		Rest = Rcurrent; 
		Vest = 0;
	}
	else
	{
		Assert(m_Tpred != 0);

		Rest = m_Rpred + m_a*(Rcurrent - m_Rpred); // Smoothing location
		Vest = m_Vpred + (m_b/Ts)*(Rcurrent - m_Rpred);
	}

	// Calculating Prediction values for next round
	m_Rpred = Rest + Vest*Ts;
	m_Vpred = Vest;
	m_Tpred = Tcurrent;

	return Rest;
}

double CDistanceSmoothingBasicAlgorithm::CalcTS(double CurrentTickCount)
{
	if (m_Tpred == 0)
		return 0;

	double DeltaInMilisec = CurrentTickCount - m_Tpred;
	double DeltaInSec = DeltaInMilisec / 1000;

	m_LastTS = DeltaInSec;

	return DeltaInSec;
}