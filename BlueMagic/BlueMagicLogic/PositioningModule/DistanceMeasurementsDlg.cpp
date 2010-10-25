// DistanceMeasurementsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PositioningModule.h"
#include "DistanceMeasurementsDlg.h"
#include "Common/Config.h"
#include "Common/collectionhelper.h"

const char* ConfigSection	= "DistanceMeasurementsParams";
const char* Prefix			= "Sensor";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CDistanceMeasurementsDlg dialog

CDistanceMeasurementsDlg::CDistanceMeasurementsDlg(CWnd* pParent /*=NULL*/)
	: CTabDlg(CDistanceMeasurementsDlg::IDD, pParent)
	, m_SmoothingParamA(0)
	, m_SmoothingParamB(0)
{

}

CDistanceMeasurementsDlg::~CDistanceMeasurementsDlg()
{
}

void CDistanceMeasurementsDlg::DoDataExchange(CDataExchange* pDX)
{
	CTabDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MEASUREMENTS_LIST, m_DistanceMesaurementsList);
	DDX_Text(pDX, IDC_A_PARAM_EDIT, m_SmoothingParamA);
	DDX_Text(pDX, IDC_B_PARAM_EDIT, m_SmoothingParamB);
}


BEGIN_MESSAGE_MAP(CDistanceMeasurementsDlg, CTabDlg)
	ON_BN_CLICKED(IDC_BUTTON1, &CDistanceMeasurementsDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CDistanceMeasurementsDlg message handlers

void CDistanceMeasurementsDlg::InitScanList()
{
	LVCOLUMN Column;
	Column.mask = LVCF_TEXT | LVCF_WIDTH;

	int Ind = 0;

#define ADD_COL(Width, Name) { Column.cx = Width; Column.pszText = (LPSTR)Name; m_DistanceMesaurementsList.InsertColumn(Ind++, &Column); }
	ADD_COL(30, "sID");
	ADD_COL(90, "BDADDRESS");
	ADD_COL(50, "RSSI");
	ADD_COL(60, "Distance");
	ADD_COL(60, "Smoothed");
	ADD_COL(60, "Velocity");
	ADD_COL(60, "TS");
#undef ADD_COL
	m_DistanceMesaurementsList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void CDistanceMeasurementsDlg::OnGuiThread(WPARAM wParam)
{
	SDialogDataMessage *Message = (SDialogDataMessage *)wParam;
	Assert(Message->m_MessageType == DialogDataMessage);

	int ListEntryIndex = GetListEntryIndex(Message);

	if (ListEntryIndex == -1)
		AddNewEntry(Message);
	else
		UpdateEntry(ListEntryIndex, Message);

	RedrawWindow();

	delete Message;
}

void CDistanceMeasurementsDlg::SaveData()
{
	WriteConfigDouble(ConfigSection, "a", m_SmoothingParamA);
	WriteConfigDouble(ConfigSection, "b", m_SmoothingParamB);
}


void CDistanceMeasurementsDlg::LoadData()
{
	m_SmoothingParamA = GetConfigDouble(ConfigSection, "a", 1.0);
	m_SmoothingParamB = GetConfigDouble(ConfigSection, "b", 1.0);

	UpdateData(FALSE);

	std::vector<ConfigListItem> ObjectSections;
	GetListSection(ConfigSection, Prefix, ObjectSections);
	//////////////////////////////////////////////////////////////////////////

	if (ObjectSections.size() == 0)
	{
		LogEvent(LE_WARNING, "Failed to read Sensor's DistanceMeasurementsConstants from Configuration !");
		return;
	}

	for (int i = 0; (unsigned)i < ObjectSections.size(); ++i) 
	{
		std::string ObjectSection = ObjectSections[i].ItemValue;
		
		// Read: SensorID, A, N for each Sensor:
		int SensorID = GetIntValue(ObjectSection.c_str(), "SensorID=", -1);//GetConfigSectionId(ConfigSection);   
		double A = GetDoubleValue(ObjectSection.c_str(), "A=", ILLEGAL_A);
		double N = GetDoubleValue(ObjectSection.c_str(), "N=", ILLEGAL_N);

		if (SensorID <= 0)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": illegal SensorID %d", SensorID);
			continue;
		}

		if (A == ILLEGAL_A)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": illegal A value (%f) for SensorID %d", A, SensorID);
			continue;
		}

		if (N == ILLEGAL_N)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": illegal N value (%f) for SensorID %d", N, SensorID);
			continue;
		}

		CRssiToDistanceBasicAlgorithm DistanceAlgorithm(A, N);

		// Add to table (should not already exist):
		if (!InsertValueToMap(m_DistanceAlgorithms, SensorID, DistanceAlgorithm))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add SensorID %d DistanceAlgorithm to map. Do you have Duplicate SensorIDs in map?", SensorID);
		}

		CDistanceSmoothingBasicAlgorithmManager SmoothingAlgorithm(m_SmoothingParamA, m_SmoothingParamB);

		// Add to table (should not already exist):
		if (!InsertValueToMap(m_SmoothingAlgorithms, SensorID, SmoothingAlgorithm))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add SensorID %d SmoothingAlgorithm map. Do you have Duplicate SensorIDs in map?", SensorID);
		}
	}

	if (m_DistanceAlgorithms.size() == 0)
	{
		LogEvent(LE_WARNING, __FUNCTION__ ": No DistanceMeasurementsConstants were read");
		return;
	}

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Created DistanceMeasurementsConstants were read for %d sensors", m_DistanceAlgorithms.size());
}


void CDistanceMeasurementsDlg::AddNewEntry(SDialogDataMessage *Message)
{
	UINT iItem = GetIndexForNewEntry(Message);

	CString strSensorID;
	strSensorID.Format("%d", Message->m_SensorId);

	m_DistanceMesaurementsList.InsertItem(iItem, strSensorID);

	UpdateEntry(iItem, Message);
}


int CDistanceMeasurementsDlg::GetListEntryIndex(SDialogDataMessage *Message)
{
#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

	CString strNewSensorID;
	strNewSensorID.Format("%d", Message->m_SensorId);

	for (int i = 0; i < m_DistanceMesaurementsList.GetItemCount(); i++)
	{
		m_DistanceMesaurementsList.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
		CString SensorID = ChrStr;

		m_DistanceMesaurementsList.GetItemText(i, 1, ChrStr, MAX_CHRSTR_LENGTH);
		CString BDADDRESS = ChrStr;

		if (SensorID == strNewSensorID 
			&& BDADDRESS == Message->m_ScannedData.ScannedBDADDRESS.c_str())
		{
			return i;
		}
	}

	return -1;
#undef MAX_CHRSTR_LENGTH
}

void CDistanceMeasurementsDlg::UpdateEntry(int index, SDialogDataMessage *Message)
{
	LVITEM NewItem;
	NewItem.mask = LVIF_TEXT;
	NewItem.iItem = index;

	int Ind = 1;

	CString strRSSI;
	strRSSI.Format("%dDb", Message->m_ScannedData.RSSI);

	double Distance = CalcDistance(Message->m_SensorId, Message->m_ScannedData.RSSI);
	CString strDistance;
	strDistance.Format("%.02fm", Distance);

	double Velocity, TS;
	double SmoothedDistance = SmoothDistance(Message->m_SensorId, Distance, Message->m_ScannedData.ScannedBDADDRESS, Message->m_ScannedData.Time, Velocity, TS);	
	CString strSmoothDistance;
	strSmoothDistance.Format("%.02fm", SmoothedDistance);
	CString strVelocity;
	strVelocity.Format("%.02fm/s", Velocity);
	CString strTS;
	strTS.Format("%.02fs", TS);

#define ADD_ITEM(Str) { NewItem.iSubItem = Ind++; NewItem.pszText = (LPSTR)(LPCSTR)Str; m_DistanceMesaurementsList.SetItem(&NewItem); }
	ADD_ITEM(Message->m_ScannedData.ScannedBDADDRESS.c_str());
	ADD_ITEM(strRSSI);
	ADD_ITEM(strDistance);
	ADD_ITEM(strSmoothDistance);
	ADD_ITEM(strVelocity);
	ADD_ITEM(strTS);
#undef ADD_ITEM	
}


int CDistanceMeasurementsDlg::GetIndexForNewEntry(SDialogDataMessage *Message)
{
#define MAX_CHRSTR_LENGTH 64
	char ChrStr[MAX_CHRSTR_LENGTH];

	for (int i = 0; i < m_DistanceMesaurementsList.GetItemCount(); i++)
	{
		m_DistanceMesaurementsList.GetItemText(i, 0, ChrStr, MAX_CHRSTR_LENGTH);
		int SensorIDValue = atoi(ChrStr);

		m_DistanceMesaurementsList.GetItemText(i, 1, ChrStr, MAX_CHRSTR_LENGTH);
		CString BDADDRESS = ChrStr;

		Assert(Message->m_SensorId != SensorIDValue 
			|| Message->m_ScannedData.ScannedBDADDRESS.c_str() != BDADDRESS);

		if (Message->m_SensorId <= SensorIDValue 
			&& Message->m_ScannedData.ScannedBDADDRESS.c_str() < BDADDRESS)
		{
			return i;
		}
	}

	return m_DistanceMesaurementsList.GetItemCount();
#undef MAX_CHRSTR_LENGTH
}

#define GET_ALGORITHM(AlgorithmClass, AlgorithmMap, Algorithm, SensorID)\
AlgorithmClass *Algorithm;										\
if (!GetValueInMap(AlgorithmMap, SensorID, Algorithm, false))	\
{																\
	LogEvent(LE_WARNING, __FUNCTION__ ": Received data from Sensor %d, yet %s does not exist in map (perhaps A & N parameters are not defined)",\
		SensorID, #AlgorithmClass);								\
	return -1;													\
}

double CDistanceMeasurementsDlg::CalcDistance(int SensorID, int RSSI)
{
	GET_ALGORITHM(CRssiToDistanceBasicAlgorithm, m_DistanceAlgorithms, Algorithm, SensorID)

	double Distance = Algorithm->CalcDistance(RSSI);
	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Distance Calculated for Sensor %d: RSSI %dDb -> Distance %.02fm (A=%f,N=%f)", SensorID, RSSI, Distance, Algorithm->GetA(), Algorithm->GetN());
	return Distance;
}

double CDistanceMeasurementsDlg::SmoothDistance(int SensorID, double Distance, std::string BDADDRESS, DWORD Time, double &Velocity, double &TS)
{
	GET_ALGORITHM(CDistanceSmoothingBasicAlgorithmManager, m_SmoothingAlgorithms, Algorithm, SensorID)

	double SmoothedDistance = Algorithm->SmoothDistance(BDADDRESS, Distance, Time);
	Velocity = Algorithm->GetEstimatedVelocity(BDADDRESS);
	TS = Algorithm->GetLastTS(BDADDRESS);

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Distance Smoothed for Sensor %d: Distance %.02fm -> Distance %.02fm (a=%f,b=%f). V=%.02fm/s, TS=%.02fs", 
		SensorID, Distance, SmoothedDistance, Algorithm->Geta(), Algorithm->Getb(), Velocity, TS);

	return SmoothedDistance;
}
void CDistanceMeasurementsDlg::OnBnClickedButton1()
{
	UpdateData(TRUE);

	std::map<int /*SensorID*/, CDistanceSmoothingBasicAlgorithmManager>::iterator Iter = m_SmoothingAlgorithms.begin();
	std::map<int /*SensorID*/, CDistanceSmoothingBasicAlgorithmManager>::iterator End = m_SmoothingAlgorithms.end();

	for(;Iter != End; ++Iter)
	{
		CDistanceSmoothingBasicAlgorithmManager &Algorithm = Iter->second;
		Algorithm.Init(m_SmoothingParamA, m_SmoothingParamB);
	}
}
