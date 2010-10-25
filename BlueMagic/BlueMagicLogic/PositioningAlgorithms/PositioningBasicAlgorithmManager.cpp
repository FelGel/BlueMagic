#include "StdAfx.h"
#include "PositioningBasicAlgorithmManager.h"
#include "Common/collectionhelper.h"
#include "Common/LogEvent.h"

#define DEFAULT_MIN_NUMBER_OF_PARTICIPATING_SENSORS 3
#define DEFAULT_MAX_ACCEPTABLE_ERROR				1.0 //meters
#define DEFAULT_MAX_NUMBER_OF_ITERATIONS			100

CPositioningBasicAlgorithmManager::CPositioningBasicAlgorithmManager(void) 
	:	m_MinNumberOfParticipatingSensor(DEFAULT_MIN_NUMBER_OF_PARTICIPATING_SENSORS), 
		m_MaxAcceptablePositioningError(DEFAULT_MAX_ACCEPTABLE_ERROR),
		m_MaxNumberOfIterations(DEFAULT_MAX_NUMBER_OF_ITERATIONS) {}

CPositioningBasicAlgorithmManager::~CPositioningBasicAlgorithmManager(void) {}

#define GET_POSITIONING_ALGORITHM_FOR_BDADDRESS(algorithm, BDADDRESS, CREATE)\
CPositioningBasicAlgorithm *algorithm;										\
if (!GetValueInMap(m_DetectedBDaddresses, BDADDRESS, algorithm, CREATE))	\
{																			\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to %s CPositioningBasicAlgorithm for %s",\
		(CREATE) ? "Create" : "Get from Map", BDADDRESS.c_str());			\
	return InvalidPosition;												\
}	


void CPositioningBasicAlgorithmManager::Init(
	std::map<int /*SensorID*/, SPosition> TheSensorsLocationMap, 
	SPosition InitialPosition, 
	double MaxAcceptablePositioningError, double MaxNumberOfIterations,
	int MinNumberOfParticipatingSensor)
{
	m_TheSensorsLocationMap = TheSensorsLocationMap;
	m_InitialPosition = InitialPosition;
	m_MaxAcceptablePositioningError = MaxAcceptablePositioningError;
	m_MaxNumberOfIterations = MaxNumberOfIterations;
	m_MinNumberOfParticipatingSensor = MinNumberOfParticipatingSensor;
}


/*virtual*/ std::map<int /*SensorID*/, SPosition> *CPositioningBasicAlgorithmManager::GetSensorsLocationMap()
{
	return &m_TheSensorsLocationMap;
}

/*virtual*/ double CPositioningBasicAlgorithmManager::GetMaxAcceptablePositioningError() const
{
	return m_MaxAcceptablePositioningError;
}

/*virtual*/ double CPositioningBasicAlgorithmManager::GetMaxNumberOfIterations() const
{
	return m_MaxNumberOfIterations;
}

/*virtual */int CPositioningBasicAlgorithmManager::GetMinNumberOfParticipatingSensor() const
{
	return m_MinNumberOfParticipatingSensor;
}

SPosition CPositioningBasicAlgorithmManager::CalcPosition(std::string BDADDRESS, std::map<int /*SensorID*/, double /*Distance*/> Measuremnts)
{
	// This must be done, and auto creation via GET_DISTANCE_SMOOTHING_ALGORITHM_FOR_BDADDRESS 
	// cannot be used for first creation as Initiating CTOR must be used (or Init called right after)
	if (!IsValueInMap(m_DetectedBDaddresses, BDADDRESS))
	{
		CPositioningBasicAlgorithm algorithm(this, m_InitialPosition);
		if (!InsertValueToMap(m_DetectedBDaddresses, BDADDRESS, algorithm))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add a Positioning Basic Algorithm. BDADDRESS %s", BDADDRESS.c_str());
			return InvalidPosition;
		}
	}

	GET_POSITIONING_ALGORITHM_FOR_BDADDRESS(algorithm, BDADDRESS, false);

	return algorithm->CalcPosition(Measuremnts);
}