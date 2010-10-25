// SensorsStatusDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "SensorsStatusDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CSensorsStatusDlg dialog


CSensorsStatusDlg::CSensorsStatusDlg(CWnd* pParent /*=NULL*/)
	: CTabDlg(CSensorsStatusDlg::IDD, pParent)
{

}

CSensorsStatusDlg::~CSensorsStatusDlg()
{
}

void CSensorsStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CTabDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SENSORS_LIST, m_SensorsListCtrl);
}


BEGIN_MESSAGE_MAP(CSensorsStatusDlg, CTabDlg)
END_MESSAGE_MAP()


// CSensorsStatusDlg message handlers


void CSensorsStatusDlg::InitScanList()
{
	LVCOLUMN Column;
	Column.mask = LVCF_TEXT | LVCF_WIDTH;

	int Ind = 0;

#define ADD_COL(Width, Name) { Column.cx = Width; Column.pszText = (LPSTR)Name; m_SensorsListCtrl.InsertColumn(Ind++, &Column); }
	ADD_COL(25, "ID");
	ADD_COL(80, "Connection");
	ADD_COL(100, "Handshake");
	ADD_COL(60,	"Activity");
	ADD_COL(120, "Last Data Received");
#undef ADD_COL
	m_SensorsListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}


void CSensorsStatusDlg::OnGuiThread(WPARAM wParam)
{
	SDialogMessage *Message = (SDialogMessage *)wParam;
	Assert(Message->m_MessageType == DialogDataMessage 
		|| Message->m_MessageType == DialogSensorMessage);

// 	if (m_OneEntryPerBDADDRESS)
// 		RemoveOldSensorEntry(Message);

	int SensorEntryIndex = GetSensorEntryIndex(Message);

	if (SensorEntryIndex == -1)
		AddNewSensorEntry(Message );
	else
		UpdateSensorEntry(SensorEntryIndex, Message);

	RedrawWindow();

	delete Message;
}

void CSensorsStatusDlg::AddNewSensorEntry(SDialogMessage *Message)
{
	UINT iItem = GetIndexForNewSensor(Message->m_SensorId);

	CString strSensorID;
	strSensorID.Format("%d", Message->m_SensorId);
	
	m_SensorsListCtrl.InsertItem(iItem, strSensorID);

	UpdateSensorEntry(iItem, Message);
}


int CSensorsStatusDlg::GetSensorEntryIndex(SDialogMessage *Message)
{
	#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

	CString strNewSensorID;
	strNewSensorID.Format("%d", Message->m_SensorId);

	for (int i = 0; i < m_SensorsListCtrl.GetItemCount(); i++)
	{
		m_SensorsListCtrl.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
		CString SensorID = ChrStr;

		if (SensorID == strNewSensorID)
		{
			return i;
		}
	}

	return -1;
	#undef MAX_CHRSTR_LENGTH
}

void CSensorsStatusDlg::UpdateSensorEntry(int index, SDialogMessage *Message)
{
	LVITEM NewItem;
	NewItem.mask = LVIF_TEXT;
	NewItem.iItem = index;

	int Ind = 1;

	if (Message->m_MessageType == DialogSensorMessage)
	{
		SDialogSensorMessage *pDialogSensorMessage = (SDialogSensorMessage *)Message;

		CString strConnection = (pDialogSensorMessage->m_IsController) ? 
			SSensorInformation::SensorConnectionStatusToShortString(pDialogSensorMessage->m_SensorConnectionStatus).c_str() : "Remote";

		CString strHandshake = SSensorInformation::SensorHandshakeStatusToShortString(pDialogSensorMessage->m_SensorHandshakeStatus).c_str();
		CString strActivity = SSensorInformation::SensorActivityStatusToShortString(pDialogSensorMessage->m_SensorActivityStatus).c_str();

		#define ADD_ITEM(Str) { NewItem.iSubItem = Ind++; NewItem.pszText = (LPSTR)(LPCSTR)Str; m_SensorsListCtrl.SetItem(&NewItem); }
		ADD_ITEM(strConnection);
		ADD_ITEM(strHandshake);
		ADD_ITEM(strActivity);
	}
	else if (Message->m_MessageType == DialogDataMessage)
	{
		SDialogDataMessage *pDialogDataMessage = (SDialogDataMessage *)Message;

		Ind = 4;
		ADD_ITEM(pDialogDataMessage->m_TimeStamp);
		#undef ADD_ITEM	
	}
	else
	{
		Assert(false);
	}
	
}


int CSensorsStatusDlg::GetIndexForNewSensor(int SensorID)
{
#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

	for (int i = 0; i < m_SensorsListCtrl.GetItemCount(); i++)
	{
		m_SensorsListCtrl.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
		int SensorIDValue = atoi(ChrStr);

		Assert(SensorID != SensorIDValue);

		if (SensorID <= SensorIDValue)
		{
			return i;
		}
	}

	return m_SensorsListCtrl.GetItemCount();
#undef MAX_CHRSTR_LENGTH
}