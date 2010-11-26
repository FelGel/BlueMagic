// PositioningEstimationDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "DepartmentEstimationDialog.h"

#include "Common/collectionhelper.h"

const char* DepartmentsFilesDirectory = "..\\ScanFiles";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define USER_HALF_RECT_LEN				5
#define HALF_TEXT_WIDTH					50
#define HALF_TIMESTAMP_TEXT_WIDTH		30
#define TEXT_HEIGHT						14
#define HALF_TEXT_HEIGHT				(TEXT_HEIGHT / 2)
#define NAME_TEXT_DIST_FROM_USER		(USER_HALF_RECT_LEN + 3)
#define TIMESTAMP_TEXT_DIST_FROM_USER	(NAME_TEXT_DIST_FROM_USER + TEXT_HEIGHT)


// CDepartmentEstimationDialog dialog

IMPLEMENT_DYNAMIC(CDepartmentEstimationDialog, CTabDlg)

CDepartmentEstimationDialog::CDepartmentEstimationDialog(CWnd* pParent /*=NULL*/)
: CTabDlg(CDepartmentEstimationDialog::IDD, pParent), m_LastCleanTickCount(0)
{

}

CDepartmentEstimationDialog::~CDepartmentEstimationDialog()
{
}

void CDepartmentEstimationDialog::DoDataExchange(CDataExchange* pDX)
{
	CTabDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_POSITION_ESTIMATIONS_LIST, m_PositionEstimationsList);
	DDX_Control(pDX, IDC_GRAPH_FRAME, m_GraphFrame);
}


BEGIN_MESSAGE_MAP(CDepartmentEstimationDialog, CTabDlg)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CDepartmentEstimationDialog::InitScanList()
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
	ADD_COL(30, "In?");
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

	CreatePositioningFilesDirectory();
	CreatePositioningFile();
}

// CDepartmentEstimationDialog message handlers


void CDepartmentEstimationDialog::OnGuiThread(WPARAM wParam)
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

void CDepartmentEstimationDialog::HandleEstablishmentContourMessage(SDialogEstablishmentContourMessage *Message)
{
	if (Message->m_EstablishmentCoordinates.size() < 3)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Establishment coordinates has less than 3 coordinates !!");
		return;
	}

	m_EstablishmentCoordinates = Message->m_EstablishmentCoordinates;
	m_DepartmentsInfo = Message->m_DepartmentsInfo;

	// Init m_PhysicalEstablishmentRect:
	// ---------------------------------
	m_PhysicalEstablishmentRect = GetEncapsulatingRect(Message->m_EstablishmentCoordinates);
}

CRectReal CDepartmentEstimationDialog::GetEncapsulatingRect(std::vector<SPosition> Coordinates)
{
	CRectReal RectReal;

	RectReal.left = Coordinates[0].x;
	RectReal.right = Coordinates[0].x;
	RectReal.top = Coordinates[0].y;
	RectReal.bottom = Coordinates[0].y;

	for (unsigned int i = 1; i < Coordinates.size(); i++)
	{
		RectReal.left = min(RectReal.left, Coordinates[i].x);
		RectReal.right = max(RectReal.right, Coordinates[i].x);
		RectReal.top = min(RectReal.top, Coordinates[i].y);
		RectReal.bottom = max(RectReal.bottom, Coordinates[i].y);
	}

	return RectReal;
}

void CDepartmentEstimationDialog::HandlePositioningMessage(SDialogPositioingMessage *Message)
{
	Assert(Message->m_MessageType == DialogPositioningMessage);

	int ListEntryIndex = GetListEntryIndex(Message);

	if (ListEntryIndex == -1)
		AddNewEntry(Message);
	else
		UpdateEntry(ListEntryIndex, Message);

	UpdateUserLocation(Message->m_BDADDRESS, Message->m_EstimatedPosition);
}

void CDepartmentEstimationDialog::AddNewEntry(SDialogPositioingMessage *Message)
{
	UINT iItem = GetIndexForNewEntry(Message);

	m_PositionEstimationsList.InsertItem(iItem, Message->m_BDADDRESS.c_str());

	UpdateEntry(iItem, Message);
}


int CDepartmentEstimationDialog::GetListEntryIndex(SDialogPositioingMessage *Message)
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

void CDepartmentEstimationDialog::UpdateEntry(int index, SDialogPositioingMessage *Message)
{
	LVITEM NewItem;
	NewItem.mask = LVIF_TEXT;
	NewItem.iItem = index;

	int Ind = 1;

	Assert(Message->m_Measurements.size() == 3);
	Assert(Message->m_DistanceEstimations.size() == 3);

#define ADD_ITEM(Str) { NewItem.iSubItem = Ind++; NewItem.pszText = (LPSTR)(LPCSTR)Str; m_PositionEstimationsList.SetItem(&NewItem); }
	std::string strAllRSSIs;
	{
		std::map<int , SMeasurement>::iterator Iter = Message->m_Measurements.begin();
		std::map<int , SMeasurement>::iterator End = Message->m_Measurements.end();
		for(;Iter != End; ++Iter)
		{
			CString strRSSI;
			//strRSSI.Format("%dDb", Iter->second.m_RSSI);
			strRSSI.Format("%d", Iter->second.m_RSSI);
			ADD_ITEM(strRSSI);
			strAllRSSIs += strRSSI + ", ";
		}
	}

	std::string strAllDistances;
	{
		std::map<int , double>::iterator Iter = Message->m_DistanceEstimations.begin();
		std::map<int , double>::iterator End = Message->m_DistanceEstimations.end();
		for(;Iter != End; ++Iter)
		{
			CString strDistance;
			//strDistance.Format("%.02fm", Iter->second);
			strDistance.Format("%.02f", Iter->second);
			ADD_ITEM(strDistance);
			strAllDistances += strDistance + ", ";
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

	CString strIsInEstablishment;
	strIsInEstablishment = (Message->m_IsInEstablishment) ? "In" : "Out";
	ADD_ITEM(strIsInEstablishment);

#undef ADD_ITEM	


	CString strSensorID;
	strSensorID.Format("%d", Message->m_SensorId);
	WriteToFile(strSensorID, strAllRSSIs.c_str(), strAllDistances.c_str(), 
		strPosition, strError, strIterations);
}


int CDepartmentEstimationDialog::GetIndexForNewEntry(SDialogPositioingMessage *Message)
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


void CDepartmentEstimationDialog::OnPaint()
{
	CPaintDC dc(this);

	DrawBackground(dc);
	DrawDepartments(dc);
	DrawEstablishment(dc);
	DrawSensors(dc);
	DrawUserPositions(dc);

	CTabDlg::OnPaint();
}

void CDepartmentEstimationDialog::DrawBackground(CPaintDC &dc)
{
	CBrush brush(RGB(200,200,200));
	dc.FillRect(m_CanvasRect, &brush);
	brush.DeleteObject();
}

void CDepartmentEstimationDialog::DrawEstablishment(CPaintDC &dc)
{
	DrawDepartment(dc, m_EstablishmentCoordinates);
}

void CDepartmentEstimationDialog::DrawDepartments(CPaintDC &dc)
{
	for (unsigned int i = 0; i < m_DepartmentsInfo.size(); i++)
	{
		DrawDepartment(dc, m_DepartmentsInfo[i].DepartmentCoordinates, m_DepartmentsInfo[i].DepartmentName);
	}
}

void CDepartmentEstimationDialog::DrawDepartment(CPaintDC &dc, std::vector<SPosition> Coordinates, std::string DerpartmentName /*= ""*/)
{
	// First, Draw Contour
	// -------------------
	if (Coordinates.size() == 0)
		return;

	CBrush brush(3, RGB(255,255,255));	
	dc.SelectObject(&brush); 

	dc.MoveTo(ConvertPhysicalCoordinateToCanvas(Coordinates[0]));

	for (unsigned int i = 1; i < Coordinates.size(); i++)
	{
		dc.LineTo(ConvertPhysicalCoordinateToCanvas(Coordinates[i]));
	}

	dc.LineTo(ConvertPhysicalCoordinateToCanvas(Coordinates[0]));

	brush.DeleteObject();

	if (DerpartmentName == "")
		return; // this is the establishment itself and does not require name printing

	// Then, Print Title
	// -----------------
	CRectReal RectReal = GetEncapsulatingRect(Coordinates);
	SPosition DepartmentNamePosition;
	
	DepartmentNamePosition.x = (RectReal.left + RectReal.right) / 2;
	DepartmentNamePosition.y = (RectReal.top + RectReal.bottom) / 2;

	POINT DepartmentNamePositionOnCanvas = ConvertPhysicalCoordinateToCanvas(DepartmentNamePosition);

	DrawText(dc, DerpartmentName.c_str(), RGB(255,255,255), 
		DepartmentNamePositionOnCanvas.x - HALF_TEXT_WIDTH,
		DepartmentNamePositionOnCanvas.y - HALF_TEXT_HEIGHT,
		DepartmentNamePositionOnCanvas.x + HALF_TEXT_WIDTH,
		DepartmentNamePositionOnCanvas.y + HALF_TEXT_HEIGHT);
}

POINT CDepartmentEstimationDialog::ConvertPhysicalCoordinateToCanvas(SPosition Coordinate)
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

void CDepartmentEstimationDialog::DrawUserPositions(CPaintDC &dc)
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

void CDepartmentEstimationDialog::DrawUserPosition(CPaintDC &dc, std::string BDADDRESS, SUserPosition UserPosition)
{
	POINT UserPositionOnCanvas = ConvertPhysicalCoordinateToCanvas(UserPosition.Position);

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

void CDepartmentEstimationDialog::DrawSquare(CPaintDC &dc, COLORREF Color, int l, int t, int r, int b)
{
	CRect UserRect(l, t, r, b);

	CBrush brush(Color);
	dc.FillRect(UserRect, &brush);
	brush.DeleteObject();
}

void CDepartmentEstimationDialog::DrawText(CPaintDC &dc, CString Text, COLORREF Color, int l, int t, int r, int b)
{
	CRect TextRect(l, t, r, b);

	dc.SetTextColor(Color);
	dc.SetBkMode( TRANSPARENT );
	dc.DrawText(Text, &TextRect, DT_SINGLELINE | DT_CENTER);
}

void CDepartmentEstimationDialog::UpdateUserLocation(std::string BDADDRESS, SPosition NewPosition)
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

void CDepartmentEstimationDialog::HandleSensorsLocationMessage(SDialogSensorsLocationMessage *Message)
{
	m_SensorsLocation = Message->m_SensorsLocation;
}

void CDepartmentEstimationDialog::DrawSensors(CPaintDC &dc)
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

void CDepartmentEstimationDialog::DrawSensor(CPaintDC &dc, int SensorID, SPosition Position)
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

CString CDepartmentEstimationDialog::GetLocalTimeForTickCount(DWORD TickCount)
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

void CDepartmentEstimationDialog::HandleTimeOut()
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

		if (RemoveList.size() > 0)
		{
			for (unsigned int i = 0; i < RemoveList.size(); i++)
			{
				RemoveValueFromMap(m_UserPositions, RemoveList[i]);
			}

			RedrawWindow();
		}
	}
}


void CDepartmentEstimationDialog::CreatePositioningFilesDirectory()
{
	CFileStatus status;

	if (CFile::GetStatus(DepartmentsFilesDirectory, status) 
		&& (status.m_attribute & CFile::directory))
	{
		LogEvent(LE_INFOLOW, __FUNCTION__ ": Directory %s already exists", DepartmentsFilesDirectory);
	} 
	else if (!CreateDirectory(DepartmentsFilesDirectory, NULL))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to create directory %s", DepartmentsFilesDirectory);
	}
}

void CDepartmentEstimationDialog::CreatePositioningFile()
{
	/* TEMP -> Write to File*/
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	CString FileName;
	FileName.Format("%s\\POSITIONING TABLE %02d.%02d.%02d %02d-%02d-%02d.csv", 
		DepartmentsFilesDirectory,
		SystemTime.wDay, SystemTime.wMonth, SystemTime.wYear, 
		SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);

	if (!CreateFile(FileName, &m_PositioningFile))
		return;

	WriteToFile("SensorID", "RSSI1, RSSI2, RSS3, ", 
		"Distance1, Distance2, Distance3, ", "X, Y, ", "dX, dY, ", "Iterations");

	LogEvent(LE_INFO, __FUNCTION__ ": File %s created successfully", FileName);
	/////////////////////////
}


bool CDepartmentEstimationDialog::CreateFile(CString FileName, CStdioFile *ScanFile)
{
	if (!ScanFile->Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText /*| CFile::shareDenyWrite*/))
	{
		DWORD err = GetLastError();
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to open %s!! ErrorCode = %d", FileName, err);
		return false;
	}

	return true;
}


void CDepartmentEstimationDialog::ClosePositioningFile()
{
	m_PositioningFile.Close();
}

void CDepartmentEstimationDialog::WriteToFile(
	CString strSensorID, CString strAllRSSIs, CString strAllDistances, 
	CString strPosition, CString strError, CString strIterations)
{
	CString DataString;
	DataString.Format("%s, %s%s%s%s%s\n",
		strSensorID, strAllRSSIs, strAllDistances, strPosition, strError, 
		strIterations);

	m_PositioningFile.WriteString(DataString);
}

void CDepartmentEstimationDialog::Close()
{
	ClosePositioningFile();
}