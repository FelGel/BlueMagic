// PositioningModule.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "PositioningModuleDlg.h"
#include "PositioningAlgorithms/PositioningAlgorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// CPositioningModuleApp

BEGIN_MESSAGE_MAP(CPositioningModuleApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CPositioningModuleApp construction
const char* APP_VERSION = "0.1";

CPositioningModuleApp::CPositioningModuleApp()  :
CResourcedGuiApplication("PositioningModuleApp", APP_VERSION, IDR_MAINFRAME),
	m_PositioningModuleRealTimeDlg(true), 
	m_PositioningModuleAggregatedDlg(false)
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CPositioningModuleApp object

void CPositioningModuleApp::AddDialogs()
{
	CResourcedGuiApplication::AddDialog(m_PositioningModuleRealTimeDlg, CPositioningModuleDlg::IDD, "Scan Status");
	CResourcedGuiApplication::AddDialog(m_PositioningModuleAggregatedDlg, CPositioningModuleDlg::IDD, "Scan Log");
	CResourcedGuiApplication::AddDialog(m_SensorsStatusDlg, CSensorsStatusDlg::IDD, "Sensors Status");
	CResourcedGuiApplication::AddDialog(m_DistancesMeasurementsDlg, CDistanceMeasurementsDlg::IDD, "Distances");
	CResourcedGuiApplication::AddDialog(m_PositioningEstimationDlg, CPositioningEstimationDlg::IDD, "Positioning");
	CResourcedGuiApplication::AddDialog(m_DepartmentEstimationDlg, CDepartmentEstimationDialog::IDD, "Department");
	return;
}

bool CPositioningModuleApp::PerformInitalization()
{
	// NOTE: ORDER IS IMPORTANT !!
	CPositioningAlgorithms::SetConfigFileNameDLL(GetConfigFileName().c_str());
	CPositioningAlgorithms::SetCrashFileNameDLL(GetApplicationName());
	CPositioningAlgorithms::SetTheLogManagerDLL(GetTheLogManager());
	CPositioningAlgorithms::SetLogEventOutputDLL(GuiLogOutput, true, GetGuiLogOutputSeverity, GetApplicationName());

	m_PositioningModuleRealTimeDlg.InitScanList();
	m_PositioningModuleAggregatedDlg.InitScanList();
	m_SensorsStatusDlg.InitScanList();
	m_DistancesMeasurementsDlg.InitScanList();
	m_PositioningEstimationDlg.InitScanList();
	m_DepartmentEstimationDlg.InitScanList();

	m_PositioningManager.Advise(this);
	return m_PositioningManager.Init();
}

bool CPositioningModuleApp::PerformCleanup()
{
	//m_PositioningModuleDlg
	m_PositioningManager.CloseThread(true);
	m_DistancesMeasurementsDlg.Close();
	m_PositioningEstimationDlg.Close();
	m_DepartmentEstimationDlg.Close();
	return true;
}

CPositioningModuleApp theApp;

void CPositioningModuleApp::SendMessageToDialog(SDialogDataMessage *Message)
{
	SDialogDataMessage *CopyMessage = new SDialogDataMessage(Message->m_SensorId, Message->m_ScannedData, Message->m_TimeStamp);
	SDialogDataMessage *CopyMessage2 = new SDialogDataMessage(Message->m_SensorId, Message->m_ScannedData, Message->m_TimeStamp);
	SDialogDataMessage *CopyMessage3 = new SDialogDataMessage(Message->m_SensorId, Message->m_ScannedData, Message->m_TimeStamp);

	m_PositioningModuleRealTimeDlg.SendMessageToGuiThread((WPARAM)Message);
	m_PositioningModuleAggregatedDlg.SendMessageToGuiThread((WPARAM)CopyMessage);
	m_SensorsStatusDlg.SendMessageToGuiThread((WPARAM)CopyMessage2);
	m_DistancesMeasurementsDlg.SendMessageToGuiThread((WPARAM)CopyMessage3);
}

void CPositioningModuleApp::SendMessageToDialog(SDialogSensorMessage *Message)
{
	m_SensorsStatusDlg.SendMessageToGuiThread((WPARAM)Message);
}

void CPositioningModuleApp::SendMessageToDialog(SDialogPositioingMessage *Message)
{
	SDialogPositioingMessage *CopyMessage = new SDialogPositioingMessage(
		Message->m_BDADDRESS, Message->m_Measurements, 
		Message->m_DistanceEstimations, Message->m_EstimatedPosition, 
		Message->m_EstimatedPositionError, Message->m_NumOfIterations, 
		Message->m_IsInEstablishment);

	m_PositioningEstimationDlg.SendMessageToGuiThread((WPARAM)Message);
	m_DepartmentEstimationDlg.SendMessageToGuiThread((WPARAM)CopyMessage);
}

void CPositioningModuleApp::SendMessageToDialog(SDialogEstablishmentContourMessage *Message)
{
	SDialogEstablishmentContourMessage *CopyMessage = new SDialogEstablishmentContourMessage(
		Message->m_EstablishmentCoordinates, Message->m_DepartmentsInfo);

	m_PositioningEstimationDlg.SendMessageToGuiThread((WPARAM)Message);
	m_DepartmentEstimationDlg.SendMessageToGuiThread((WPARAM)CopyMessage);
}

void CPositioningModuleApp::SendMessageToDialog(SDialogSensorsLocationMessage *Message)
{
	SDialogSensorsLocationMessage *CopyMessage = new SDialogSensorsLocationMessage(
		Message->m_SensorsLocation);

	m_PositioningEstimationDlg.SendMessageToGuiThread((WPARAM)Message);
	m_DepartmentEstimationDlg.SendMessageToGuiThread((WPARAM)CopyMessage);
}

void CPositioningModuleApp::OnTimeoutCalledFromPositioningManager()
{
	m_PositioningEstimationDlg.OnTimeoutCalledFromPositioningModule();
	m_DepartmentEstimationDlg.OnTimeoutCalledFromPositioningModule();
}