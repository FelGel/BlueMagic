#include "StdAfx.h"
#include "PositioningManager.h"
#include "CuidGenerator.h"
#include "Common/collectionhelper.h"

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

void CPositioningManager::OnSensorInfo(int /*SensorId*/, SSensorInfo /*SensorInfo*/)
{

}


void CPositioningManager::OnIncomingScannedData(int SensorId, SScannedData ScannedData)
{
	AddHandlerToQueue(&CPositioningManager::HandleDataReceived, SensorId, ScannedData);

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

	/* TEMP -> Write to File*/
	CloseAllScanFiles();
}

void CPositioningManager::HandleDataReceived(const int &SensorId, const SScannedData& ScannedData)
{
	m_PositioningAlgorithm.OnScannedData(SensorId, ScannedData);

	/* TEMP -> Write to File*/
	UpdateScanFile(SensorId, ScannedData);
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

void CPositioningManager::OnSensorInSystem(int SensorId, bool IsController, std::string BDADDRESS, std::vector<int> ChildrenSensorIDs)
{
	AddHandlerToQueue(&CPositioningManager::HandleNewSensorInSystem, SensorId, IsController, BDADDRESS, ChildrenSensorIDs);	
}

void CPositioningManager::HandleNewSensorInSystem(const int &SensorId, const bool &IsController, const std::string &BDADDRESS, const std::vector<int> &ChildrenSensorIDs)
{
	// ToDo - In future, perhaps hold SensorInformation.. (alternately, ask it from SensorController each time (either through queue (multi-task) or direct & dirty)
	LogEvent(LE_INFOHIGH, __FUNCTION__ ": New %s in System. SensorId = %d, BDADDRESS = %s, Children = %s",
		(IsController) ? "Sensor CONTROLLER" : "REMOTE Sensor", SensorId, BDADDRESS.c_str(), 
		(ChildrenSensorIDs.size() == 0) ? "None" : IntVectorToStr(ChildrenSensorIDs).c_str());

	/* TEMP -> Write to File*/
	CreateScanFile(SensorId);
}

void CPositioningManager::CreateScanFile(const int SensorId)
{
	/* TEMP -> Write to File*/
	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);

	CString FileName;
	FileName.Format("ScanFile_Sensor%d %d.%d.%d %d-%d-%d.csv", 
		SensorId, 
		SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear, 
		SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	CStdioFile *ScanFile = new CStdioFile;
	if (!ScanFile->Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText /*| CFile::shareDenyWrite*/))
	{
		DWORD err = GetLastError();
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to open %s!! ErrorCode = %d", FileName, err);
		return;
	}

	if (!InsertValueToMap(m_ScanFiles, SensorId, ScanFile))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add File %s to Map ! Do you have error in configuration?", FileName);
		return;
	}

	LogEvent(LE_INFO, __FUNCTION__ ": File %s created successfully", FileName);
	/////////////////////////
}

void CPositioningManager::UpdateScanFile(const int &SensorId, const SScannedData& ScannedData)
{
	/* TEMP -> Write to File*/
	CString DataString;

	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);

	DataString.Format("%s, %d, %d:%d:%d\n", ScannedData.ScannedBDADDRESS.c_str(), ScannedData.RSSI,
		SystemTime.wHour,SystemTime.wMinute,SystemTime.wSecond);

	CStdioFile *ScanFile;
	if (!GetValueFromMap(m_ScanFiles, SensorId, ScanFile))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to find ScanFile for SensorID %d in Map !!", SensorId);
		return;
	}

	ScanFile->WriteString(DataString);
	/////////////////////////
}

void CPositioningManager::CloseAllScanFiles()
{
	/* TEMP -> Write to File*/
	std::map<int /*SensorID*/, CStdioFile*>::iterator Iter = m_ScanFiles.begin();
	std::map<int /*SensorID*/, CStdioFile*>::iterator End = m_ScanFiles.end();

	for(;Iter != End; ++Iter)
	{
		((CStdioFile*)Iter->second)->Close(); // close file

		delete Iter->second;
		Iter->second = NULL;
	}

	m_ScanFiles.clear();
	///////////////////////
}