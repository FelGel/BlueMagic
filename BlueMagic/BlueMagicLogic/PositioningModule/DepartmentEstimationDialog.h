#pragma once

#include "..\ApplicationUtils/TabDlg.h"
#include "PositioningManager.h"
#include "PositioningEstimationDlg.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "RectReal.h"

// CDepartmentEstimationDialog dialog

class CDepartmentEstimationDialog : public CTabDlg
{
	DECLARE_DYNAMIC(CDepartmentEstimationDialog)

public:
	CDepartmentEstimationDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDepartmentEstimationDialog();
	afx_msg void OnPaint();

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}
	void Close();

	virtual void OnGuiThread(WPARAM /*wParam*/);

	void OnTimeoutCalledFromPositioningModule() {GoToGuiThread(NULL);}

	// Dialog Data
	enum { IDD = IDD_DEPARTMENT_ESTIMATION_DIALOG };

private:
	void HandleEstablishmentContourMessage(SDialogEstablishmentContourMessage *Message);
	void HandlePositioningMessage(SDialogPositioingMessage *Message);
	void HandleSensorsLocationMessage(SDialogSensorsLocationMessage *Message);
	void HandleTimeOut();

	void AddNewEntry(SDialogPositioingMessage *Message);
	void UpdateEntry(int index, SDialogPositioingMessage *Message);
	int GetListEntryIndex(SDialogPositioingMessage *Message);
	int GetIndexForNewEntry(SDialogPositioingMessage *Message);
	void UpdateUserLocation(std::string BDADDRESS, SPosition NewPosition);

	void DrawBackground(CPaintDC &dc);
	void DrawEstablishment(CPaintDC &dc);
	void DrawDepartments(CPaintDC &dc);
	void DrawSensors(CPaintDC &dc);
	void DrawSensor(CPaintDC &dc, int SensorID, SPosition Position);
	void DrawUserPositions(CPaintDC &dc);
	void DrawUserPosition(CPaintDC &dc, std::string BDADDRESS, SUserPosition UserPosition);
	
	POINT ConvertPhysicalCoordinateToCanvas(SPosition Coordinate);
	CRectReal GetEncapsulatingRect(std::vector<SPosition> Coordinates);

	void DrawSquare(CPaintDC &dc, COLORREF Color, int l, int t, int r, int b);
	void DrawText(CPaintDC &dc, CString Text, COLORREF Color, int l, int t, int r, int b);
	void DrawDepartment(CPaintDC &dc, std::vector<SPosition> Coordinates, std::string DerpartmentName = "");

	CString GetLocalTimeForTickCount(DWORD TickCount);

	void CreatePositioningFilesDirectory();
	void CreatePositioningFile();
	void ClosePositioningFile();
	bool CreateFile(CString FileName, CStdioFile *ScanFile);
	void WriteToFile(
		CString strSensorID, CString strAllRSSIs, CString strAllDistances, 
		CString strPosition, CString strError, CString strIterations);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_PositionEstimationsList;
	CStatic m_GraphFrame;

	std::vector<SPosition> m_EstablishmentCoordinates;
	std::vector<SDepartmentInfo> m_DepartmentsInfo;
	CRectReal m_PhysicalEstablishmentRect;
	CRect m_CanvasRect;

	std::map<std::string /*BDADDRESS*/, SUserPosition> m_UserPositions;
	std::map<int /*SensorID*/, SPosition> m_SensorsLocation;

	DWORD m_LastCleanTickCount;

	CStdioFile m_PositioningFile;
};
