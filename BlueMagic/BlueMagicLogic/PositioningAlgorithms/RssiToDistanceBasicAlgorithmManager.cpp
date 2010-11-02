#include "StdAfx.h"
#include "RssiToDistanceBasicAlgorithmManager.h"

#include "Common/collectionhelper.h"
#include "PositionStructure.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CRssiToDistanceBasicAlgorithmManager::CRssiToDistanceBasicAlgorithmManager(void) {}
CRssiToDistanceBasicAlgorithmManager::~CRssiToDistanceBasicAlgorithmManager(void) {}


#define GET_RSSI_TO_DISTANCE_ALGORITHM_FOR_SENSORID(algorithm, SensorID, CREATE, RetIfFailed)\
	CRssiToDistanceBasicAlgorithm *algorithm;										\
	if (!GetValueInMap(m_SensorsAlgorithms, SensorID, algorithm, CREATE))			\
{																					\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to %s RssiToDistanceBasicAlgorithm for SensorID %d",\
	(CREATE) ? "Create" : "Get from Map", SensorID);								\
	return RetIfFailed;																\
}	

bool CRssiToDistanceBasicAlgorithmManager::Init(std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams> SensorsParams)
{
	std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams>::iterator Iter = SensorsParams.begin();
	std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams>::iterator End = SensorsParams.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		SRssiToDistanceBasicAlgorithmParams &SensorParams = Iter->second;

		if (IsValueInMap(m_SensorsAlgorithms, SensorID))
		{	
			LogEvent(LE_ERROR, __FUNCTION__ ": Params for SensorID %d are already in map!! Do you have duplicate lines in config?",
				SensorID);
			continue;
		}

		GET_RSSI_TO_DISTANCE_ALGORITHM_FOR_SENSORID(algorithm, SensorID, true, false)

		algorithm->Init(SensorParams.A, SensorParams.N);
	}

	return true;
}

double CRssiToDistanceBasicAlgorithmManager::CalcDistance(int SensorID, int RSSI)
{
	GET_RSSI_TO_DISTANCE_ALGORITHM_FOR_SENSORID(algorithm, SensorID, true, INVALID_MEASUREMENT)

	return algorithm->CalcDistance(RSSI);
}

double CRssiToDistanceBasicAlgorithmManager::GetA(int SensorID)
{
	GET_RSSI_TO_DISTANCE_ALGORITHM_FOR_SENSORID(algorithm, SensorID, true, ILLEGAL_A)

	return algorithm->GetA();
}

double CRssiToDistanceBasicAlgorithmManager::GetN(int SensorID)
{
	GET_RSSI_TO_DISTANCE_ALGORITHM_FOR_SENSORID(algorithm, SensorID, true, ILLEGAL_N)

	return algorithm->GetN();
}