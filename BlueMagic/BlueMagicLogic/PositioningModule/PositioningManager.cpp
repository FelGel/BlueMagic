#include "StdAfx.h"
#include "PositioningManager.h"
#include "CuidGenerator.h"

#define POSITION_MANAGER_QUEUE_SIZE 10000
#define POSITION_MANAGER_THREAD_TIMEOUT 100 //milisec

const char* SensorControllersConfigurationSection = "SensorControllers";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPositioningManager::CPositioningManager(void) : CThreadWithQueue("PositioningManager", POSITION_MANAGER_QUEUE_SIZE)
{
	m_PositioningInterfaceHandler = NULL;
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

	if (!m_EstablishmentTopology.Init())
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not Initialize Establishment Topology!!");
		return false;
	}

	m_PositioningAlgorithm.Advise(&m_EstablishmentTopology, this);

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

void CPositioningManager::OnErrorInTopology(UCHAR /*SensorId*/)
{

}

void CPositioningManager::OnSensorInfo(SSensorInfo /*SensorInfo*/)
{

}

void CPositioningManager::OnSensorsInfo(CList<SSensorInfo> * /*SensorsInfo*/)
{

}

void CPositioningManager::OnIncomingScannedData(CList<SScannedData> *ScannedDataList)
{
	while (!ScannedDataList->IsEmpty())
	{
		SScannedData ScannedData = (*ScannedDataList).GetHead();
		AddHandlerToQueue(&CPositioningManager::HandleDataReceived, ScannedData);

		ScannedDataList->RemoveHead();
	}

	// MARKED IN PURPOSE !! ScannedData will be deleted later by the SensorController!
	// delete ScannedData;
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

void CPositioningManager::HandleDataReceived(const SScannedData& ScannedData)
{
	m_PositioningAlgorithm.OnScannedData(ScannedData);
}

void CPositioningManager::OnPositioning(std::string BDADDRESS, SPosition Position, double Accuracy, DWORD TimeStamp, int StoreID, bool IsInStore)
{
	// NOTE: This function is running in local thread! 
	// No need to add to queue !!
	if (m_PositioningInterfaceHandler != NULL)
	{
		std::string CUID = CCuidGenerator::ConvertToCuid(BDADDRESS);
		m_PositioningInterfaceHandler->OnPosition(CUID, Position, Accuracy, TimeStamp, StoreID, IsInStore);
	}
}