#pragma once

#include "..\ApplicationUtils/TabDlg.h"
#include "PositioningAlgorithms\RssiToDistanceBasicAlgorithm.h"
#include "PositioningAlgorithms\DistanceSmoothingBasicAlgorithmManager.h"
#include "afxcmn.h"
#include <map>

// CDistanceMeasurementsDlg dialog

class CDistanceMeasurementsDlg : public CTabDlg
{
public:
	CDistanceMeasurementsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDistanceMeasurementsDlg();

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}
	void Close();

	virtual void OnGuiThread(WPARAM /*wParam*/);
	virtual void LoadData();
	virtual void SaveData();

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
	double SmoothDistance(int SensorID, double Distance, std::string BDADDRESS, DWORD Time, double &Velocity, double &TS);

	void CreateDistanceFilesDirectory();
	void CreateDistanceFile();
	void CloseDistanceFile();
	bool CreateFile(CString FileName, CStdioFile *ScanFile);
	void WriteToFile(
		CString strSensorID, CString strBDADDRESS, 
		CString strTimeStamp, CString strRSSI,
		CString strDistance, CString strSmoothDistance, 
		CString strVelocity, CString strTS);
	
private:
	std::map<int /*SensorID*/, CRssiToDistanceBasicAlgorithm> m_DistanceAlgorithms;
	std::map<int /*SensorID*/, CDistanceSmoothingBasicAlgorithmManager> m_SmoothingAlgorithms;

// DLG PARAMS:
	CListCtrl m_DistanceMesaurementsList;
	double m_SmoothingParamA;
	double m_SmoothingParamB;
	CStdioFile m_DistanceFile;
public:
	afx_msg void OnBnClickedButton1();
};
