#pragma once

#include "..\ApplicationUtils/TabDlg.h"
#include "PositioningManager.h"
#include "afxcmn.h"


// CSensorsStatusDlg dialog

class CSensorsStatusDlg : public CTabDlg
{
public:
	CSensorsStatusDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSensorsStatusDlg();

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}

	virtual void OnGuiThread(WPARAM /*wParam*/);

// Dialog Data
	enum { IDD = IDD_SENSOR_STATUS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	void AddNewSensorEntry(SDialogMessage *Message);
	void UpdateSensorEntry(int index, SDialogMessage *Message);
	int GetSensorEntryIndex(SDialogMessage *Message);
	int GetIndexForNewSensor(int SensorID);
	

private:
	CListCtrl m_SensorsListCtrl;
};
