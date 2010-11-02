#include "StdAfx.h"
#include "PositioningManager.h"
#include "CuidGenerator.h"
#include "Common/collectionhelper.h"
#include "DialogMessages.h"

#define POSITION_MANAGER_QUEUE_SIZE 10000
#define POSITION_MANAGER_THREAD_TIMEOUT 100 //milisec

const char* SensorControllersConfigurationSection = "SensorControllers";
const char* ScanFilesDirectory = "..\\ScanFiles";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CPositioningManager::CPositioningManager(void) : CThreadWithQueue("PositioningManager", POSITION_MANAGER_QUEUE_SIZE)
{
	m_PositioningInterfaceHandler = NULL;
	m_DialogMessagesInterfaceHandler = NULL;
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
	
	if (!m_PositioningAlgorithm.Init())
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not Initialize Positioning Algorithm!!");
		return false;
	}

	CreateScanFilesDirectory();

	CreateCombinedScanFile();

	// start thread
	SetTimeout(POSITION_MANAGER_THREAD_TIMEOUT);
	bool Success = StartThread();
	if (!Success)
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not start working thread !!");
		return false;
	}

	LogEvent(LE_NOTICE, __FUNCTION__ ": Positioning Manager Initialized Successfully");

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

void CPositioningManager::OnSensorStatusUpdate(int SensorId, bool IsController, ESensorConnectionStatus SensorConnectionStatus, ESensorHandshakeStatus SensorHandshakeStatus, ESensorActivityStatus SensorActivityStatus)
{
	AddHandlerToQueue(&CPositioningManager::HandleSensorStatusUpdate, SensorId, IsController, SensorConnectionStatus, SensorHandshakeStatus, SensorActivityStatus);
}


void CPositioningManager::HandleSensorStatusUpdate(const int &SensorId, const bool &IsController, const ESensorConnectionStatus &SensorConnectionStatus, const ESensorHandshakeStatus &SensorHandshakeStatus, const ESensorActivityStatus &SensorActivityStatus)
{
	SDialogSensorMessage *DialogMessage = new SDialogSensorMessage(SensorId, IsController, SensorConnectionStatus, SensorHandshakeStatus, SensorActivityStatus);

	m_DialogMessagesInterfaceHandler->SendMessageToDialog(DialogMessage);
}

void CPositioningManager::OnThreadClose()
{
	m_SensorControllersContainer.RemoveObjects();

	m_PositioningAlgorithm.Close();

	/* TEMP -> Write to File*/
	CloseAllScanFiles();
}

void CPositioningManager::HandleDataReceived(const int &SensorId, const SScannedData& ScannedData)
{
	m_PositioningAlgorithm.OnScannedData(SensorId, ScannedData);

	/* TEMP -> Write to File*/
	UpdateScanFile(SensorId, ScannedData);
	UpdateDialog(SensorId, ScannedData);
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


void CPositioningManager::CreateScanFilesDirectory()
{
	CFileStatus status;

	if (CFile::GetStatus(ScanFilesDirectory, status) 
		&& (status.m_attribute & CFile::directory))
	{
		LogEvent(LE_INFOLOW, __FUNCTION__ ": Directory %s already exists", ScanFilesDirectory);
	} 
	else if (!CreateDirectory(ScanFilesDirectory, NULL))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to create directory %s", ScanFilesDirectory);
	}
}

void CPositioningManager::CreateCombinedScanFile()
{
	/* TEMP -> Write to File*/
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	CString FileName;
	FileName.Format("%s\\Combined ScanFile %02d.%02d.%02d %02d-%02d-%02d.csv", 
		ScanFilesDirectory, SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear, 
		SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	if (!CreateScanFile(FileName, &m_CombinedScanFiles))
		return;

	LogEvent(LE_INFO, __FUNCTION__ ": File %s created successfully", FileName);
	/////////////////////////
}

void CPositioningManager::CreateScanFile(const int SensorId)
{
	/* TEMP -> Write to File*/
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	CString FileName;
	FileName.Format("%s\\ScanFile_Sensor%d %02d.%02d.%02d %02d-%02d-%02d.csv", 
		ScanFilesDirectory, SensorId, 
		SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear, 
		SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	CStdioFile *ScanFile = new CStdioFile;
	if (!CreateScanFile(FileName, ScanFile))
		return;
	
	if (!InsertValueToMap(m_ScanFiles, SensorId, ScanFile))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add File %s to Map ! Do you have error in configuration?", FileName);
		return;
	}

	LogEvent(LE_INFO, __FUNCTION__ ": File %s created successfully", FileName);
	/////////////////////////
}

bool CPositioningManager::CreateScanFile(CString FileName, CStdioFile *ScanFile)
{
	if (!ScanFile->Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText /*| CFile::shareDenyWrite*/))
	{
		DWORD err = GetLastError();
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to open %s!! ErrorCode = %d", FileName, err);
		return false;
	}

	return true;
}

void CPositioningManager::UpdateScanFile(const int &SensorId, const SScannedData& ScannedData)
{
	/* TEMP -> Write to File*/
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	CString DataString;
	DataString.Format("%s, %d, %02d:%02d:%02d\n", ScannedData.ScannedBDADDRESS.c_str(), ScannedData.RSSI,
		SystemTime.wHour,SystemTime.wMinute,SystemTime.wSecond);

	CStdioFile *ScanFile;
	if (!GetValueFromMap(m_ScanFiles, SensorId, ScanFile))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to find ScanFile for SensorID %d in Map !!", SensorId);
		return;
	}

	ScanFile->WriteString(DataString);


	CString CombinedDataString;
	CombinedDataString.Format("%d, %s, %d, %02d:%02d:%02d\n", SensorId, ScannedData.ScannedBDADDRESS.c_str(), ScannedData.RSSI,
		SystemTime.wHour,SystemTime.wMinute,SystemTime.wSecond);
	m_CombinedScanFiles.WriteString(CombinedDataString);
	/////////////////////////
}

void CPositioningManager::UpdateDialog(const int &SensorId, const SScannedData& ScannedData)
{
	if (m_DialogMessagesInterfaceHandler != NULL)
	{
		SYSTEMTIME SystemTime;
		GetLocalTime(&SystemTime);

		CString TimeStamp;
		TimeStamp.Format("%02d:%02d:%02d", SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

		SDialogDataMessage *DialogMessage = new SDialogDataMessage(SensorId, ScannedData, TimeStamp);

		m_DialogMessagesInterfaceHandler->SendMessageToDialog(DialogMessage);
	}

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

	m_CombinedScanFiles.Close();
	///////////////////////
}

/*virtual*/ void CPositioningManager::OnTimeout()
{
	m_PositioningAlgorithm.OnTimeout();
}