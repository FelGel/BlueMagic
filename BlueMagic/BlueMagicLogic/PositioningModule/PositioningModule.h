// PositioningModule.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols
#include "../ApplicationUtils/ResourcedGuiApplication.h"
#include "PositioningModuleDlg.h"
#include "SensorsStatusDlg.h"
#include "DialogMessages.h"
#include "DistanceMeasurementsDlg.h"
#include "PositioningEstimationDlg.h"
#include "DepartmentEstimationDialog.h"

// CPositioningModuleApp:
// See PositioningModule.cpp for the implementation of this class
//

class CPositioningModuleApp : public CResourcedGuiApplication, public IDialogMessagesInterface
{
public:
	CPositioningModuleApp();

	virtual void SendMessageToDialog(SDialogDataMessage *Message);
	virtual void SendMessageToDialog(SDialogSensorMessage *Message);
	virtual void SendMessageToDialog(SDialogPositioingMessage *Message);
	virtual void SendMessageToDialog(SDialogEstablishmentContourMessage *Message);
	virtual void SendMessageToDialog(SDialogSensorsLocationMessage *Message);

	virtual void OnTimeoutCalledFromPositioningManager();

// Overrides
	//public:
	//virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
private:
	virtual void AddDialogs();
	virtual bool PerformInitalization();
	virtual bool PerformCleanup();

private:
	CPositioningManager m_PositioningManager;
	CPositioningModuleDlg m_PositioningModuleRealTimeDlg;
	CPositioningModuleDlg m_PositioningModuleAggregatedDlg;
	CSensorsStatusDlg     m_SensorsStatusDlg;
	CDistanceMeasurementsDlg m_DistancesMeasurementsDlg;
	CPositioningEstimationDlg m_PositioningEstimationDlg;
	CDepartmentEstimationDialog m_DepartmentEstimationDlg;
};

extern CPositioningModuleApp theApp;
