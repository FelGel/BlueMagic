#include "StdAfx.h"
#include "DistanceSmoothingBasicAlgorithmManager.h"

#include "Common/collectionhelper.h"
#include "Common/LogEvent.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CDistanceSmoothingBasicAlgorithmManager::CDistanceSmoothingBasicAlgorithmManager(void) : m_a(0), m_b(0){}
CDistanceSmoothingBasicAlgorithmManager::CDistanceSmoothingBasicAlgorithmManager(double a, double b) : m_a(a), m_b(b){}
CDistanceSmoothingBasicAlgorithmManager::~CDistanceSmoothingBasicAlgorithmManager(void) {}

#define GET_DISTANCE_SMOOTHING_ALGORITHM_FOR_BDADDRESS(algorithm, BDADDRESS, CREATE)\
CDistanceSmoothingBasicAlgorithm *algorithm;									\
if (!GetValueInMap(m_DetectedBDaddresses, BDADDRESS, algorithm, CREATE))	\
{																			\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to %s DistanceSmoothingBasicAlgorithm for %s",\
		(CREATE) ? "Create" : "Get from Map", BDADDRESS.c_str());			\
	return INVALID_MEASUREMENT;												\
}																			

void CDistanceSmoothingBasicAlgorithmManager::Init(double a, double b)
{
	m_a = a;
	m_b = b;

	std::map<std::string /*BDADDRESS*/, CDistanceSmoothingBasicAlgorithm>::iterator Iter = m_DetectedBDaddresses.begin();
	std::map<std::string /*BDADDRESS*/, CDistanceSmoothingBasicAlgorithm>::iterator End = m_DetectedBDaddresses.end();

	for(;Iter != End; ++Iter)
	{
		CDistanceSmoothingBasicAlgorithm &Algorithm = Iter->second;
		Algorithm.Init(m_a, m_b);
	}
}

double CDistanceSmoothingBasicAlgorithmManager::SmoothDistance(std::string BDADDRESS, double Rcurrent, double Tcurrent)
{
	// This must be done, and auto creation via GET_DISTANCE_SMOOTHING_ALGORITHM_FOR_BDADDRESS 
	// cannot be used for first creation as Initiating CTOR must be used (or Init called right after)
	if (!IsValueInMap(m_DetectedBDaddresses, BDADDRESS))
	{
		CDistanceSmoothingBasicAlgorithm algorithm(m_a, m_b);
		if (!InsertValueToMap(m_DetectedBDaddresses, BDADDRESS, algorithm))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add a Smoothing Algorithm. BDADDRESS %s", BDADDRESS.c_str());
			return INVALID_MEASUREMENT;
		}
	}

	GET_DISTANCE_SMOOTHING_ALGORITHM_FOR_BDADDRESS(algorithm, BDADDRESS, false);

	return algorithm->SmoothDistance(Rcurrent, Tcurrent);
}

double CDistanceSmoothingBasicAlgorithmManager::GetEstimatedVelocity(std::string BDADDRESS)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_FOR_BDADDRESS(algorithm, BDADDRESS, false);

	return algorithm->GetEstimatedVelocity();
}

double CDistanceSmoothingBasicAlgorithmManager::GetLastTS(std::string BDADDRESS)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_FOR_BDADDRESS(algorithm, BDADDRESS, false);

	return algorithm->GetLastTS();
}