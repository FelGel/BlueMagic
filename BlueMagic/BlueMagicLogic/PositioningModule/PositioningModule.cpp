// PositioningModule.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "PositioningModuleDlg.h"


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
	CResourcedGuiApplication::AddDialog(m_DistancesMeasurementsDlg, CDistanceMeasurementsDlg::IDD, "Distances Measurements");

	return;
}

bool CPositioningModuleApp::PerformInitalization()
{
	m_PositioningModuleRealTimeDlg.InitScanList();
	m_PositioningModuleAggregatedDlg.InitScanList();
	m_SensorsStatusDlg.InitScanList();
	m_DistancesMeasurementsDlg.InitScanList();

	m_PositioningManager.Advise(this);
	return m_PositioningManager.Init();
}

bool CPositioningModuleApp::PerformCleanup()
{
	//m_PositioningModuleDlg
	m_PositioningManager.CloseThread(true);
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