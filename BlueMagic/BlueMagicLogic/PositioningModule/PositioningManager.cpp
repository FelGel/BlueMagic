#include "StdAfx.h"
#include "PositioningManager.h"

#define POSITION_MANAGER_QUEUE_SIZE 10000
#define POSITION_MANAGER_THREAD_TIMEOUT 100 //milisec

const char* SensorControllersConfigurationSection = "SensorControllers";

CPositioningManager::CPositioningManager(void) : CThreadWithQueue("PositioningManager", POSITION_MANAGER_QUEUE_SIZE)
{
}

CPositioningManager::~CPositioningManager(void)
{
}


bool CPositioningManager::Init()
{
	// Create Sensors & Sensors Container
	if (!m_SensorControllersContainer.CreateObjects(SensorControllersConfigurationSection, this))
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not create Sensor Controllers !!");
		return false;
	}

	// start thread
	SetTimeout(POSITION_MANAGER_THREAD_TIMEOUT);
	bool Success = StartThread();
	if (!Success)
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not start working thread !!");
		return false;
	}

	return true;
}

void CPositioningManager::OnErrorInTopolog(UCHAR /*SensorId*/)
{

}

void CPositioningManager::OnSensorInfo(SSensorInfo /*SensorInfo*/)
{

}

void CPositioningManager::OnSensorsInfo(CList<SSensorInfo> /*SensorsInfo*/)
{

}

void CPositioningManager::OnIncomingScannedData(CList<SScannedData> /*ScannedData*/)
{

}

void CPositioningManager::OnConnected(UCHAR /*SensorId*/)
{

}

void CPositioningManager::OnDisconnected(UCHAR /*SensorId*/)
{

}

void CPositioningManager::OnThreadClose()
{
	m_SensorControllersContainer.RemoveObjects();
}