// PositioningModuleDlg.h : header file
//

#pragma once


#include "..\ApplicationUtils/TabDlg.h"

// CPositioningModuleDlg dialog
class CPositioningModuleDlg : public CTabDlg
{
// Construction
public:
	CPositioningModuleDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CPositioningModuleDlg();

// Dialog Data
	enum { IDD = IDD_POSITIONINGMODULE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
