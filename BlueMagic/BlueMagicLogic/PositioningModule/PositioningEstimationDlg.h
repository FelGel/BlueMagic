#pragma once

#include "..\ApplicationUtils/TabDlg.h"
#include "PositioningManager.h"
#include "afxcmn.h"

// CPositioningEstimationDlg dialog

class CPositioningEstimationDlg : public CTabDlg
{
	DECLARE_DYNAMIC(CPositioningEstimationDlg)

public:
	CPositioningEstimationDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPositioningEstimationDlg();

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}

	virtual void OnGuiThread(WPARAM /*wParam*/);

// Dialog Data
	enum { IDD = IDD_POSITIONING_ESTIMATION_DIALOG };

private:
	void AddNewEntry(SDialogPositioingMessage *Message);
	void UpdateEntry(int index, SDialogPositioingMessage *Message);
	int GetListEntryIndex(SDialogPositioingMessage *Message);
	int GetIndexForNewEntry(SDialogPositioingMessage *Message);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_PositionEstimationsList;
};
