// PositioningModuleDlg.h : header file
//

#pragma once


#include "..\ApplicationUtils/TabDlg.h"
#include "PositioningManager.h"
#include "afxcmn.h"

// CPositioningModuleDlg dialog
class CPositioningModuleDlg : public CTabDlg
{
// Construction
public:
	CPositioningModuleDlg(bool OneEntryPerBDADDRESS, CWnd* pParent = NULL);	// standard constructor
	virtual ~CPositioningModuleDlg();

// Dialog Data
	enum { IDD = IDD_POSITIONINGMODULE_DIALOG };

	void InitScanList();
	void SendMessageToGuiThread(WPARAM wParam) {GoToGuiThread(wParam);}
	
	virtual void LoadData();
	virtual void OnGuiThread(WPARAM /*wParam*/);

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

private:
	void AddNewScanEntry(SDialogDataMessage *Message);
	void RemoveOldScanEntry(SDialogDataMessage *Message);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	/*virtual BOOL OnInitDialog();*/
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	CListCtrl m_ScanListCtrl;
	bool m_OneEntryPerBDADDRESS;
	//int m_NextAvailableItemIndex;
};
