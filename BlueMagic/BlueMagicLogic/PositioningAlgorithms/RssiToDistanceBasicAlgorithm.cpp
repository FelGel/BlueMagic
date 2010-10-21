#include "StdAfx.h"
#include "RssiToDistanceBasicAlgorithm.h"

#include "math.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CRssiToDistanceBasicAlgorithm::CRssiToDistanceBasicAlgorithm(void) : m_A(ILLEGAL_A), m_N(ILLEGAL_N) {}
CRssiToDistanceBasicAlgorithm::CRssiToDistanceBasicAlgorithm(double A, double N) : m_A(A), m_N(N) {}
CRssiToDistanceBasicAlgorithm::~CRssiToDistanceBasicAlgorithm(void) {}


void CRssiToDistanceBasicAlgorithm::Init(double A, double N)
{
	m_A = A;
	m_N = N;
}

double CRssiToDistanceBasicAlgorithm::CalcDistance(int RSSI) const
{
	return CalcDistance(RSSI, m_A, m_N);
}

/*static*/ double CRssiToDistanceBasicAlgorithm::CalcDistance(int RSSI, double A, double N)
{
	double Distance = pow(10,(A-RSSI)/(10*N));
	return Distance;
}