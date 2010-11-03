// PositioningEstimationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "PositioningEstimationDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPositioningEstimationDlg dialog

IMPLEMENT_DYNAMIC(CPositioningEstimationDlg, CTabDlg)

CPositioningEstimationDlg::CPositioningEstimationDlg(CWnd* pParent /*=NULL*/)
	: CTabDlg(CPositioningEstimationDlg::IDD, pParent)
{

}

CPositioningEstimationDlg::~CPositioningEstimationDlg()
{
}

void CPositioningEstimationDlg::DoDataExchange(CDataExchange* pDX)
{
	CTabDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_POSITION_ESTIMATIONS_LIST, m_PositionEstimationsList);
}


BEGIN_MESSAGE_MAP(CPositioningEstimationDlg, CTabDlg)
END_MESSAGE_MAP()

void CPositioningEstimationDlg::InitScanList()
{
	LVCOLUMN Column;
	Column.mask = LVCF_TEXT | LVCF_WIDTH;

	int Ind = 0;

#define ADD_COL(Width, Name) { Column.cx = Width; Column.pszText = (LPSTR)Name; m_PositionEstimationsList.InsertColumn(Ind++, &Column); }
	ADD_COL(90, "BDADDRESS");
	ADD_COL(35, "Rx1");
	ADD_COL(35, "Rx2");
	ADD_COL(35, "Rx3");
	ADD_COL(40, "D1");
	ADD_COL(40, "D2");
	ADD_COL(40, "D3");
	ADD_COL(70, "Position");
	ADD_COL(75, "Error");
	ADD_COL(40, "#Iter");
#undef ADD_COL
	m_PositionEstimationsList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

// CPositioningEstimationDlg message handlers


void CPositioningEstimationDlg::OnGuiThread(WPARAM wParam)
{
	SDialogPositioingMessage *Message = (SDialogPositioingMessage *)wParam;
	Assert(Message->m_MessageType == DialogPositioningMessage);

	int ListEntryIndex = GetListEntryIndex(Message);

	if (ListEntryIndex == -1)
		AddNewEntry(Message);
	else
		UpdateEntry(ListEntryIndex, Message);

	RedrawWindow();

	delete Message;
}


void CPositioningEstimationDlg::AddNewEntry(SDialogPositioingMessage *Message)
{
	UINT iItem = GetIndexForNewEntry(Message);

	m_PositionEstimationsList.InsertItem(iItem, Message->m_BDADDRESS.c_str());

	UpdateEntry(iItem, Message);
}


int CPositioningEstimationDlg::GetListEntryIndex(SDialogPositioingMessage *Message)
{
#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

// 	CString strNewSensorID;
// 	strNewSensorID.Format("%d", Message->m_SensorId);
// 
// 	for (int i = 0; i < m_DistanceMesaurementsList.GetItemCount(); i++)
// 	{
// 		m_DistanceMesaurementsList.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
// 		CString SensorID = ChrStr;
// 
// 		m_DistanceMesaurementsList.GetItemText(i, 1, ChrStr, MAX_CHRSTR_LENGTH);
// 		CString BDADDRESS = ChrStr;
// 
// 		if (SensorID == strNewSensorID 
// 			&& BDADDRESS == Message->m_ScannedData.ScannedBDADDRESS.c_str())
// 		{
// 			return i;
// 		}
// 	}

	return -1;
#undef MAX_CHRSTR_LENGTH
}

void CPositioningEstimationDlg::UpdateEntry(int index, SDialogPositioingMessage *Message)
{
	LVITEM NewItem;
	NewItem.mask = LVIF_TEXT;
	NewItem.iItem = index;

	int Ind = 1;

	Assert(Message->m_Measurements.size() == 3);
	Assert(Message->m_DistanceEstimations.size() == 3);

#define ADD_ITEM(Str) { NewItem.iSubItem = Ind++; NewItem.pszText = (LPSTR)(LPCSTR)Str; m_PositionEstimationsList.SetItem(&NewItem); }
	{
		std::map<int , SMeasurement>::iterator Iter = Message->m_Measurements.begin();
		std::map<int , SMeasurement>::iterator End = Message->m_Measurements.end();
		for(;Iter != End; ++Iter)
		{
			CString strRSSI;
			//strRSSI.Format("%dDb", Iter->second.m_RSSI);
			strRSSI.Format("%d", Iter->second.m_RSSI);
			ADD_ITEM(strRSSI);
		}
	}
	
	{
		std::map<int , double>::iterator Iter = Message->m_DistanceEstimations.begin();
		std::map<int , double>::iterator End = Message->m_DistanceEstimations.end();
		for(;Iter != End; ++Iter)
		{
			CString strDistance;
			//strDistance.Format("%.02fm", Iter->second);
			strDistance.Format("%.02f", Iter->second);
			ADD_ITEM(strDistance);
		}
	}

	CString strPosition;
	strPosition.Format("%.02f, %.02f", Message->m_EstimatedPosition.x, Message->m_EstimatedPosition.y);
	ADD_ITEM(strPosition);

	CString strError;
	strError.Format("%.02f, %.02f", Message->m_EstimatedPositionError.x, Message->m_EstimatedPositionError.y);
	ADD_ITEM(strError);
	
	CString strIterations;
	strIterations.Format("%d", Message->m_NumOfIterations);
	ADD_ITEM(strIterations);

#undef ADD_ITEM	
}


int CPositioningEstimationDlg::GetIndexForNewEntry(SDialogPositioingMessage *Message)
{
#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

// 	for (int i = 0; i < m_DistanceMesaurementsList.GetItemCount(); i++)
// 	{
// 		m_DistanceMesaurementsList.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
// 		int SensorIDValue = atoi(ChrStr);
// 
// 		m_DistanceMesaurementsList.GetItemText(i, 1, ChrStr, MAX_CHRSTR_LENGTH);
// 		CString BDADDRESS = ChrStr;
// 
// 		Assert(Message->m_SensorId != SensorIDValue 
// 			|| Message->m_ScannedData.ScannedBDADDRESS.c_str() != BDADDRESS);
// 
// 		if (Message->m_SensorId <= SensorIDValue 
// 			&& Message->m_ScannedData.ScannedBDADDRESS.c_str() < BDADDRESS)
// 		{
// 			return i;
// 		}
// 	}
// 
// 	return m_DistanceMesaurementsList.GetItemCount();

	return 0;
#undef MAX_CHRSTR_LENGTH
}

