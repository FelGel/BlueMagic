#include "StdAfx.h"
#include "DistanceSmoothingBasicAlgorithmGeneralManager.h"

#include "Common/collectionhelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CDistanceSmoothingBasicAlgorithmGeneralManager::CDistanceSmoothingBasicAlgorithmGeneralManager(void) {}
CDistanceSmoothingBasicAlgorithmGeneralManager::~CDistanceSmoothingBasicAlgorithmGeneralManager(void) {}

#define GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, CREATE, RetIfFailed)\
CDistanceSmoothingBasicAlgorithmManager *manager;									\
if (!GetValueInMap(m_SensorsAlgorithms, SensorID, manager, CREATE))					\
{																					\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to %s DistanceSmoothingBasicAlgorithmManager for SensorID %d",\
	(CREATE) ? "Create" : "Get from Map", SensorID);								\
	return RetIfFailed;																\
}	

bool CDistanceSmoothingBasicAlgorithmGeneralManager::Init(std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams> SensorsParams)
{
	std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams>::iterator Iter = SensorsParams.begin();
	std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams>::iterator End = SensorsParams.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		SDistanceSmoothingAlgorithmParams &SensorParams = Iter->second;

		if (IsValueInMap(m_SensorsAlgorithms, SensorID))
		{	
			LogEvent(LE_ERROR, __FUNCTION__ ": Params for SensorID %d are already in map!! Do you have duplicate lines in config?",
				SensorID);
			continue;
		}

		GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, true, false)

		manager->Init(SensorParams.a, SensorParams.b);
	}

	return true;
}


double CDistanceSmoothingBasicAlgorithmGeneralManager::SmoothDistance(int SensorID, std::string BDADDRESS, double Rcurrent, double Tcurrent /*Current TickCount of measurement*/)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, false, INVALID_MEASUREMENT)

	return manager->SmoothDistance(BDADDRESS, Rcurrent, Tcurrent);
}

double CDistanceSmoothingBasicAlgorithmGeneralManager::Geta(int SensorID)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, false, INVALID_MEASUREMENT)
	
	return manager->Geta();
}

double CDistanceSmoothingBasicAlgorithmGeneralManager::Getb(int SensorID)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, false, INVALID_MEASUREMENT)

	return manager->Getb();
}

double CDistanceSmoothingBasicAlgorithmGeneralManager::GetEstimatedVelocity(int SensorID, std::string BDADDRESS)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, false, INVALID_MEASUREMENT)

	return manager->GetEstimatedVelocity(BDADDRESS);
}

double CDistanceSmoothingBasicAlgorithmGeneralManager::GetLastTS(int SensorID, std::string BDADDRESS)
{
	GET_DISTANCE_SMOOTHING_ALGORITHM_MANAGER_FOR_SENSORID(manager, SensorID, false, INVALID_MEASUREMENT)

	return manager->GetLastTS(BDADDRESS);
}