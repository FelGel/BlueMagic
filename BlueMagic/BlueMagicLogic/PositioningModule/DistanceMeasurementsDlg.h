#pragma once

#include "..\ApplicationUtils/TabDlg.h"
#include "afxcmn.h"
#include <map>

#define ILLEGAL_A +999999
#define ILLEGAL_N +999999

struct SSensorParameters
{
	SSensorParameters() : A(ILLEGAL_A), N(ILLEGAL_N) {}
	SSensorParameters(double _A, double _N) : A(_A), N(_N) {}
	double A;
	double N;
};

// CDistanceMeasurementsDlg dialog

class CDistanceMeasurementsDlg : public CTabDlg
{
public:
	CDistanceMeasurementsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDistanceMeasurementsDlg();

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}

	virtual void OnGuiThread(WPARAM /*wParam*/);
	virtual void LoadData();

// Dialog Data
	enum { IDD = IDD_DISTANCE_MEASUREMENTS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	void AddNewEntry(SDialogDataMessage *Message);
	void UpdateEntry(int index, SDialogDataMessage *Message);
	int GetListEntryIndex(SDialogDataMessage *Message);
	int GetIndexForNewEntry(SDialogDataMessage *Message);

	double CalcDistance(int SensorID, int RSSI);
	
public:
	CListCtrl m_DistanceMesaurementsList;
	std::map<int /*SensorID*/, SSensorParameters> m_SensorsParameters;
};
