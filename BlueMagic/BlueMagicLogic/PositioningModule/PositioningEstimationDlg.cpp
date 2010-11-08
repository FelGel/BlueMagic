// PositioningEstimationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "PositioningEstimationDlg.h"

#include "Common/collectionhelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPositioningEstimationDlg dialog

IMPLEMENT_DYNAMIC(CPositioningEstimationDlg, CTabDlg)

CPositioningEstimationDlg::CPositioningEstimationDlg(CWnd* pParent /*=NULL*/)
	: CTabDlg(CPositioningEstimationDlg::IDD, pParent), m_LastCleanTickCount(0)
{

}

CPositioningEstimationDlg::~CPositioningEstimationDlg()
{
}

void CPositioningEstimationDlg::DoDataExchange(CDataExchange* pDX)
{
	CTabDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_POSITION_ESTIMATIONS_LIST, m_PositionEstimationsList);
	DDX_Control(pDX, IDC_GRAPH_FRAME, m_GraphFrame);
}


BEGIN_MESSAGE_MAP(CPositioningEstimationDlg, CTabDlg)
	ON_WM_PAINT()
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


	// Init Canvas:
	CRect GraphFrameRect;
	m_GraphFrame.GetWindowRect(GraphFrameRect);

	ScreenToClient(&GraphFrameRect);

#define LenFromEdge 10
	m_CanvasRect = CRect(
		GraphFrameRect.left + LenFromEdge, 
		GraphFrameRect.top + LenFromEdge * 2, 
		GraphFrameRect.right - LenFromEdge, 
		GraphFrameRect.bottom - LenFromEdge);
#undef LenFromEdge
}

// CPositioningEstimationDlg message handlers


void CPositioningEstimationDlg::OnGuiThread(WPARAM wParam)
{
	if (wParam == NULL)
	{ // It's TimeOut !
		HandleTimeOut();
		return;
	}

	SDialogMessage *Message = (SDialogMessage *)wParam;

	switch (Message->m_MessageType)
	{
		case DialogPositioningMessage:
			HandlePositioningMessage((SDialogPositioingMessage *)wParam);
			break;

		case DialogEstablishmentContourMessage:
			HandleEstablishmentContourMessage((SDialogEstablishmentContourMessage *)wParam);
			break;

		case DialogSensorsLocationMessage:
			HandleSensorsLocationMessage((SDialogSensorsLocationMessage *)wParam);
			break;

		default:
			LogEvent(LE_ERROR, __FUNCTION__ ": Unexpected DialogMessage arrived!");
	}

	RedrawWindow();

	delete Message;
}

void CPositioningEstimationDlg::HandleEstablishmentContourMessage(SDialogEstablishmentContourMessage *Message)
{
	if (Message->m_EstablishmentCoordinates.size() < 3)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Establishment coordinates has less than 3 coordinates !!");
		return;
	}

	m_EstablishmentCoordinates = Message->m_EstablishmentCoordinates;

	m_PhysicalEstablishmentRect.left = Message->m_EstablishmentCoordinates[0].x;
	m_PhysicalEstablishmentRect.right = Message->m_EstablishmentCoordinates[0].x;
	m_PhysicalEstablishmentRect.top = Message->m_EstablishmentCoordinates[0].y;
	m_PhysicalEstablishmentRect.bottom = Message->m_EstablishmentCoordinates[0].y;

	for (unsigned int i = 1; i < Message->m_EstablishmentCoordinates.size(); i++)
	{
		m_PhysicalEstablishmentRect.left = min(m_PhysicalEstablishmentRect.left, Message->m_EstablishmentCoordinates[i].x);
		m_PhysicalEstablishmentRect.right = max(m_PhysicalEstablishmentRect.right, Message->m_EstablishmentCoordinates[i].x);
		m_PhysicalEstablishmentRect.top = min(m_PhysicalEstablishmentRect.top, Message->m_EstablishmentCoordinates[i].y);
		m_PhysicalEstablishmentRect.bottom = max(m_PhysicalEstablishmentRect.bottom, Message->m_EstablishmentCoordinates[i].y);
	}
}

void CPositioningEstimationDlg::HandlePositioningMessage(SDialogPositioingMessage *Message)
{
	Assert(Message->m_MessageType == DialogPositioningMessage);

	int ListEntryIndex = GetListEntryIndex(Message);

	if (ListEntryIndex == -1)
		AddNewEntry(Message);
	else
		UpdateEntry(ListEntryIndex, Message);

	UpdateUserLocation(Message->m_BDADDRESS, Message->m_EstimatedPosition);
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


void CPositioningEstimationDlg::OnPaint()
{
	CPaintDC dc(this);

	DrawBackground(dc);
 	DrawEstablishment(dc);
	DrawSensors(dc);
	DrawUserPositions(dc);

	CTabDlg::OnPaint();
}

void CPositioningEstimationDlg::DrawBackground(CPaintDC &dc)
{
	CBrush brush(RGB(200,200,200));
	dc.FillRect(m_CanvasRect, &brush);
	brush.DeleteObject();
}

void CPositioningEstimationDlg::DrawEstablishment(CPaintDC &dc)
{
	if (m_EstablishmentCoordinates.size() == 0)
		return;

	CBrush brush(3, RGB(255,255,255));	
	dc.SelectObject(&brush); 

	dc.MoveTo(ConvertPhysicalCoordinateToCanvas(m_EstablishmentCoordinates[0]));

	for (unsigned int i = 1; i < m_EstablishmentCoordinates.size(); i++)
	{
		dc.LineTo(ConvertPhysicalCoordinateToCanvas(m_EstablishmentCoordinates[i]));
	}

	dc.LineTo(ConvertPhysicalCoordinateToCanvas(m_EstablishmentCoordinates[0]));

	brush.DeleteObject();
}

POINT CPositioningEstimationDlg::ConvertPhysicalCoordinateToCanvas(SPosition Coordinate)
{
	POINT CanvasPoint;

	double DistanceFromEdge = Coordinate.x - m_PhysicalEstablishmentRect.left;
	double RelativePosition = DistanceFromEdge / m_PhysicalEstablishmentRect.Width();
	double CanvasDistanceFromEdge = RelativePosition * m_CanvasRect.Width();
	
	CanvasPoint.x = (LONG)(m_CanvasRect.left + CanvasDistanceFromEdge);

	DistanceFromEdge = Coordinate.y - m_PhysicalEstablishmentRect.top;
	RelativePosition = DistanceFromEdge / m_PhysicalEstablishmentRect.Height();
	CanvasDistanceFromEdge = RelativePosition * m_CanvasRect.Height();

	CanvasPoint.y = (LONG)(m_CanvasRect.top + CanvasDistanceFromEdge);

	return CanvasPoint;
}

void CPositioningEstimationDlg::DrawUserPositions(CPaintDC &dc)
{
	std::map<std::string /*BDADDRESS*/, SUserPosition>::iterator Iter = m_UserPositions.begin();
	std::map<std::string /*BDADDRESS*/, SUserPosition>::iterator End = m_UserPositions.end();

	for(;Iter != End; ++Iter)
	{
		std::string BDADDRESS = Iter->first;
		SUserPosition UserPosition = Iter->second;

		DrawUserPosition(dc, BDADDRESS, UserPosition);
	}

}

void CPositioningEstimationDlg::DrawUserPosition(CPaintDC &dc, std::string BDADDRESS, SUserPosition UserPosition)
{
	POINT UserPositionOnCanvas = ConvertPhysicalCoordinateToCanvas(UserPosition.Position);
	#define USER_HALF_RECT_LEN			5
	#define HALF_TEXT_WIDTH				50
	#define HALF_TIMESTAMP_TEXT_WIDTH	30
	#define TEXT_HEIGHT					15
	#define NAME_TEXT_DIST_FROM_USER USER_HALF_RECT_LEN + 3
	#define TIMESTAMP_TEXT_DIST_FROM_USER NAME_TEXT_DIST_FROM_USER + TEXT_HEIGHT
	
	DrawSquare(dc, RGB(0,0,255), 
		UserPositionOnCanvas.x - USER_HALF_RECT_LEN, 
		UserPositionOnCanvas.y - USER_HALF_RECT_LEN, 
		UserPositionOnCanvas.x + USER_HALF_RECT_LEN,
		UserPositionOnCanvas.y + USER_HALF_RECT_LEN);
	
	DrawText(dc, BDADDRESS.c_str(), RGB(0,0,200),
		UserPositionOnCanvas.x - HALF_TEXT_WIDTH,
		UserPositionOnCanvas.y + NAME_TEXT_DIST_FROM_USER,
		UserPositionOnCanvas.x + HALF_TEXT_WIDTH,
		UserPositionOnCanvas.y + NAME_TEXT_DIST_FROM_USER + TEXT_HEIGHT);

	DrawText(dc, UserPosition.TimeString, RGB(0,0,200),
		UserPositionOnCanvas.x - HALF_TIMESTAMP_TEXT_WIDTH,
		UserPositionOnCanvas.y + TIMESTAMP_TEXT_DIST_FROM_USER,
		UserPositionOnCanvas.x + HALF_TIMESTAMP_TEXT_WIDTH,
		UserPositionOnCanvas.y + TIMESTAMP_TEXT_DIST_FROM_USER + TEXT_HEIGHT);

}

void CPositioningEstimationDlg::DrawSquare(CPaintDC &dc, COLORREF Color, int l, int t, int r, int b)
{
	CRect UserRect(l, t, r, b);

	CBrush brush(Color);
	dc.FillRect(UserRect, &brush);
	brush.DeleteObject();
}

void CPositioningEstimationDlg::DrawText(CPaintDC &dc, CString Text, COLORREF Color, int l, int t, int r, int b)
{
	CRect TextRect(l, t, r, b);

	dc.SetTextColor(Color);
	dc.SetBkMode( TRANSPARENT );
	dc.DrawText(Text, &TextRect, DT_SINGLELINE);

}

void CPositioningEstimationDlg::UpdateUserLocation(std::string BDADDRESS, SPosition NewPosition)
{
	SUserPosition *UserPosition;
	if (!GetValueInMap(m_UserPositions, BDADDRESS, UserPosition, true))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": failed to UpdateUserLocation !");
		return;
	}

	UserPosition->Position.x = NewPosition.x;
	UserPosition->Position.y = NewPosition.y;
	UserPosition->TickCount = GetTickCount();
	
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	UserPosition->TimeString.Format("%02d:%02d:%02d", SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
}

void CPositioningEstimationDlg::HandleSensorsLocationMessage(SDialogSensorsLocationMessage *Message)
{
	m_SensorsLocation = Message->m_SensorsLocation;
}

void CPositioningEstimationDlg::DrawSensors(CPaintDC &dc)
{
	std::map<int /*SensorID*/, SPosition>::iterator Iter = m_SensorsLocation.begin();
	std::map<int /*SensorID*/, SPosition>::iterator End = m_SensorsLocation.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		SPosition Position = Iter->second;

		DrawSensor(dc, SensorID, Position);
	}
}

void CPositioningEstimationDlg::DrawSensor(CPaintDC &dc, int SensorID, SPosition Position)
{
	POINT UserPositionOnCanvas = ConvertPhysicalCoordinateToCanvas(Position);

	#define HALF_SENSORID_TEXT_WIDTH	5

	DrawSquare(dc, RGB(255,255,255), 
		UserPositionOnCanvas.x - USER_HALF_RECT_LEN, 
		UserPositionOnCanvas.y - USER_HALF_RECT_LEN, 
		UserPositionOnCanvas.x + USER_HALF_RECT_LEN,
		UserPositionOnCanvas.y + USER_HALF_RECT_LEN);

	CString StrSensorID;
	StrSensorID.Format("%d", SensorID);

	DrawText(dc, StrSensorID, RGB(255,255,255),
		UserPositionOnCanvas.x - HALF_SENSORID_TEXT_WIDTH,
		UserPositionOnCanvas.y + NAME_TEXT_DIST_FROM_USER,
		UserPositionOnCanvas.x + HALF_SENSORID_TEXT_WIDTH,
		UserPositionOnCanvas.y + NAME_TEXT_DIST_FROM_USER + TEXT_HEIGHT);
}

CString CPositioningEstimationDlg::GetLocalTimeForTickCount(DWORD TickCount)
{
	unsigned long uptime = TickCount;
	//unsigned int days = uptime / (24 * 60 * 60 * 1000);
	uptime %= (24 * 60 * 60 * 1000);
	unsigned int hours = uptime / (60 * 60 * 1000);
	uptime %= (60 * 60 * 1000);
	unsigned int minutes = uptime / (60 * 1000);
	uptime %= (60 * 1000);
	unsigned int seconds = uptime / (1000);

	CString LocalTime;
	LocalTime.Format("%02d:%02d:%02d", hours, minutes, seconds);

	return LocalTime;
}

#define CLEAN_TIMEOUT		60000 // remove after x mili of inactivity
#define CLEAN_REFRESH_TIME	10000 // check every X mili

void CPositioningEstimationDlg::HandleTimeOut()
{
	DWORD Now = GetTickCount();

	if (Now - m_LastCleanTickCount > CLEAN_REFRESH_TIME)
	{
		m_LastCleanTickCount = Now;

		std::map<std::string /*BDADDRESS*/, SUserPosition>::iterator Iter = m_UserPositions.begin();
		std::map<std::string /*BDADDRESS*/, SUserPosition>::iterator End = m_UserPositions.end();

		std::vector<std::string> RemoveList;
		for(;Iter != End; ++Iter)
		{
			std::string BDADDRESS = Iter->first;
			SUserPosition UserPosition = Iter->second;

			if (Now - UserPosition.TickCount > CLEAN_TIMEOUT)
				RemoveList.push_back(BDADDRESS);
		}

		for (unsigned int i = 0; i < RemoveList.size(); i++)
		{
			RemoveValueFromMap(m_UserPositions, RemoveList[i]);
		}
	}
}