#include "StdAfx.h"
#include "DistanceSmoothingBasicAlgorithm.h"
#include "Common/LogEvent.h"

#define FIRST_MEASUREMENT -1

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CDistanceSmoothingBasicAlgorithm::CDistanceSmoothingBasicAlgorithm(void) 
	: m_a(0), m_b(0), m_Rpred(FIRST_MEASUREMENT), m_Vpred(0), m_Tpred(0), m_LastTS(0), 
	m_RpredOLD(FIRST_MEASUREMENT), m_VpredOLD(0), m_TpredOLD(0), m_RcurrentOLD(0) {}
CDistanceSmoothingBasicAlgorithm::CDistanceSmoothingBasicAlgorithm(double a, double b) 
	: m_a(a), m_b(b), m_Rpred(FIRST_MEASUREMENT), m_Vpred(0), m_Tpred(0), m_LastTS(0), 
	m_RpredOLD(FIRST_MEASUREMENT), m_VpredOLD(0), m_TpredOLD(0), m_RcurrentOLD(0) {}
CDistanceSmoothingBasicAlgorithm::~CDistanceSmoothingBasicAlgorithm(void) {}

void CDistanceSmoothingBasicAlgorithm::Init(double a, double b)
{
	m_a = a;
	m_b = b;

	m_Rpred = FIRST_MEASUREMENT;
	m_Vpred = 0;
	m_Tpred = 0;
	m_LastTS = 0;

	m_RpredOLD = FIRST_MEASUREMENT;
	m_VpredOLD = 0;
	m_TpredOLD = 0;
	m_RcurrentOLD = 0;
}

double CDistanceSmoothingBasicAlgorithm::SmoothDistance(double Rcurrent, double Tcurrent)
{
	// First - Correct TS = 0
	// ======================
	// Note: Following function may change Rcurrent to previous Rcurrent,
	// as well as roll back other members
	CorrectTS0Algorithm(Rcurrent, Tcurrent);


	double Rest, Vest;
	double Ts = CalcTS(Tcurrent); /*Time Since Last Measurement*/
	Assert(m_Rpred == FIRST_MEASUREMENT || Ts != 0); // As CorrectTS0Algorithm fixed this problem

	// Calculating Current Smoothed Distance
	if (m_Rpred == FIRST_MEASUREMENT)
	{
		Assert(m_Tpred == 0);
		Assert(m_RpredOLD == FIRST_MEASUREMENT);

		// first time - no smoothing
		Rest = Rcurrent; 
		Vest = 0;
	}
	else
	{
		Assert(Ts != 0); // As CorrectTS0Algorithm fixed this problem
		Assert(m_Tpred != 0);
		// R.C.
		m_Rpred += m_Vpred*Ts; // completing prediction based on actual TS
		// ----

		Rest = m_Rpred + m_a*(Rcurrent - m_Rpred); // Smoothing location
		
		Vest = m_Vpred + (m_b/Ts)*(Rcurrent - m_Rpred);
	}

	// Calculating Prediction values for next round

	// R.C.
	// m_Rpred = Rest + Vest*Ts; - cannot predict in advance as TS changes !!
	m_Rpred = Rest; // Vest*Ts will be added with next measurement
	// ----
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

void CDistanceSmoothingBasicAlgorithm::CorrectTS0Algorithm(double &Rcurrent, double Tcurrent)
{
	if (Tcurrent != m_Tpred) // Normal case. Different TimeStamp
	{
		m_RpredOLD = m_Rpred;
		m_VpredOLD = m_Vpred;
		m_TpredOLD = m_Tpred;

		m_RcurrentOLD = Rcurrent;
	}
	else // TS = 0. Start rollback
	{
		m_Rpred = m_RpredOLD;
		m_Vpred = m_VpredOLD;
		m_Tpred = m_TpredOLD;

		if (Rcurrent > m_RcurrentOLD) 
		{
			// Current Rcurrent distance from sensor is higher,
			// meaning RSSI measurement is lower,
			// in which case previous measurement counts.
			// we should re-calculate previous measurement
			Rcurrent = m_RcurrentOLD;
		}
	}
}
