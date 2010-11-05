#pragma once

#include "..\ApplicationUtils/TabDlg.h"
#include "PositioningManager.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "RectReal.h"

// CPositioningEstimationDlg dialog

class CPositioningEstimationDlg : public CTabDlg
{
	DECLARE_DYNAMIC(CPositioningEstimationDlg)

public:
	CPositioningEstimationDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPositioningEstimationDlg();
	afx_msg void OnPaint();

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}

	virtual void OnGuiThread(WPARAM /*wParam*/);

// Dialog Data
	enum { IDD = IDD_POSITIONING_ESTIMATION_DIALOG };

private:
	void HandleEstablishmentContourMessage(SDialogEstablishmentContourMessage *Message);
	void HandlePositioningMessage(SDialogPositioingMessage *Message);

	void AddNewEntry(SDialogPositioingMessage *Message);
	void UpdateEntry(int index, SDialogPositioingMessage *Message);
	int GetListEntryIndex(SDialogPositioingMessage *Message);
	int GetIndexForNewEntry(SDialogPositioingMessage *Message);
	void UpdateUserLocation(std::string BDADDRESS, SPosition NewPosition);

	void DrawBackground(CPaintDC &dc);
	void DrawEstablishment(CPaintDC &dc);
	void DrawUserPositions(CPaintDC &dc);
	void DrawUserPosition(CPaintDC &dc, std::string BDADDRESS, SPosition Position);
	POINT ConvertPhysicalCoordinateToCanvas(SPosition Coordinate);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_PositionEstimationsList;
	CStatic m_GraphFrame;

	std::vector<SPosition> m_EstablishmentCoordinates;
	CRectReal m_PhysicalEstablishmentRect;
	CRect m_CanvasRect;

	std::map<std::string /*BDADDRESS*/, SPosition /*Position*/> m_UserPositions;
};
