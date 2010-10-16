// PositioningModuleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "PositioningModuleDlg.h"
#include "Common/Config.h"

static const char* CONFIG_SECTION = "PositioningDialogGUI";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CPositioningModuleDlg dialog




CPositioningModuleDlg::CPositioningModuleDlg(bool OneEntryPerBDADDRESS, CWnd* pParent /*=NULL*/)
	: CTabDlg(CPositioningModuleDlg::IDD, pParent), m_OneEntryPerBDADDRESS(OneEntryPerBDADDRESS)/*, m_NextAvailableItemIndex(0)*/
{
	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

/*virtual*/ CPositioningModuleDlg::~CPositioningModuleDlg()
{
}

void CPositioningModuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CTabDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCAN_LIST, m_ScanListCtrl);
}

BEGIN_MESSAGE_MAP(CPositioningModuleDlg, CTabDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CPositioningModuleDlg message handlers

//BOOL CPositioningModuleDlg::OnInitDialog()
//{
//	CTabDlg::OnInitDialog();
//
//	// Set the icon for this dialog.  The framework does this automatically
//	//  when the application's main window is not a dialog
//	SetIcon(m_hIcon, TRUE);			// Set big icon
//	SetIcon(m_hIcon, FALSE);		// Set small icon
//
//	// TODO: Add extra initialization here
//	m_PositioningManager.Init();
//
//	return TRUE;  // return TRUE  unless you set the focus to a control
//}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPositioningModuleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CTabDlg::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPositioningModuleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPositioningModuleDlg::OnGuiThread(WPARAM wParam)
{
	SDialogDataMessage *Message = (SDialogDataMessage *)wParam;
	Assert(Message->m_MessageType == DialogDataMessage);

	if (m_OneEntryPerBDADDRESS)
		RemoveOldScanEntry(Message);

	AddNewScanEntry(Message);

	RedrawWindow();

	delete Message;
}

void CPositioningModuleDlg::InitScanList()
{
	LVCOLUMN Column;
	Column.mask = LVCF_TEXT | LVCF_WIDTH;

	int Ind = 0;

#define ADD_COL(Width, Name) { Column.cx = Width; Column.pszText = (LPSTR)Name; m_ScanListCtrl.InsertColumn(Ind++, &Column); }
	ADD_COL(100, "BDADDRESS");
	ADD_COL(60,	"SensorID");
	ADD_COL(100, "Time");
	ADD_COL(40,	"RSSI");
#undef ADD_COL
	m_ScanListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	/*
	m_ScanListCtrl.InsertColumn(0, "BDADDRESS", LVCFMT_LEFT, 100, 0);
	m_ScanListCtrl.InsertColumn(1, "SensorID", LVCFMT_LEFT, 60, 1);
	m_ScanListCtrl.InsertColumn(2, "Time", LVCFMT_LEFT, 100, 2);
	m_ScanListCtrl.InsertColumn(3, "RSSI", LVCFMT_LEFT, 60, 3);
	*/
}

void CPositioningModuleDlg::AddNewScanEntry(SDialogDataMessage *Message)
{
	LVITEM NewItem;
	NewItem.mask = LVIF_TEXT;

	NewItem.iItem = 0;
	NewItem.pszText = (LPSTR)(LPCSTR)Message->m_ScannedData.ScannedBDADDRESS.c_str();
	m_ScanListCtrl.InsertItem(0, NewItem.pszText);

	CString strSensorID;
	strSensorID.Format("%d", Message->m_SensorId);
	
	CString strRSSI;
	strRSSI.Format("%d", Message->m_ScannedData.RSSI);

	int Ind = 1;
#define ADD_ITEM(Str) { NewItem.iSubItem = Ind++; NewItem.pszText = (LPSTR)(LPCSTR)Str; m_ScanListCtrl.SetItem(&NewItem); }
	ADD_ITEM(strSensorID);
	ADD_ITEM(Message->m_TimeStamp);
	ADD_ITEM(strRSSI);
#undef ADD_ITEM


	/*

	// ScannedBDADDRESS
	m_ScanListCtrl.InsertItem(LVIF_PARAM | LVIF_TEXT, m_NextAvailableItemIndex, Message->m_ScannedData.ScannedBDADDRESS.c_str(), 0, 0, 0, (LPARAM)Message);

	// SensorID
	CString strSensorID;
	strSensorID.Format("%d", Message->m_SensorId);
	m_ScanListCtrl.SetItem(m_NextAvailableItemIndex, 1, 0, strSensorID, 0, 0, 0, (LPARAM)Message);

	// TimeStamp
	m_ScanListCtrl.SetItem(m_NextAvailableItemIndex, 2, 0, Message->m_TimeStamp, 0, 0, 0, (LPARAM)Message);

	// RSSI
	CString strRSSI;
	strRSSI.Format("%d", Message->m_ScannedData.RSSI);
	m_ScanListCtrl.SetItem(m_NextAvailableItemIndex, 3, 0, Message->m_TimeStamp, 0, 0, 0, (LPARAM)Message);

	m_NextAvailableItemIndex++;
*/

}

void CPositioningModuleDlg::RemoveOldScanEntry(SDialogDataMessage *Message)
{
	#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

	CString strSensorID;
	strSensorID.Format("%d", Message->m_SensorId);

	for (int i = 0; i < m_ScanListCtrl.GetItemCount(); i++)
	{
		m_ScanListCtrl.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
		CString OldItemBDADRESS = ChrStr;
		m_ScanListCtrl.GetItemText(i, 1, ChrStr, MAX_CHRSTR_LENGTH);
		CString OldItemSensorID = ChrStr;

		if (strSensorID == OldItemSensorID 
			&& Message->m_ScannedData.ScannedBDADDRESS.c_str() == OldItemBDADRESS)
		{
			m_ScanListCtrl.DeleteItem(i);
			return;
		}
	}
	#undef MAX_CHRSTR_LENGTH
}

void CPositioningModuleDlg::LoadData()
{
	// Determined in CTOR
	//m_OneEntryPerBDADDRESS = GetConfigBool(CONFIG_SECTION, "OneEntryPerBDADDRESS", true);
}