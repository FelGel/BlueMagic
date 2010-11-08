#include "StdAfx.h"
#include "PositioningAlgorithm.h"

#include "Common/collectionhelper.h"
#include "Common/LogEvent.h"
#include "Common/Config.h"
#include "PositioningAlgorithms/PositioningAlgorithmBasicImplementation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_MIN_NUMBER_OF_SENSORS													3		//sensors
#define DEFAULT_DESIRED_NUMBER_OF_SENSORS												3		//sensors // Todo - in the future set to 4? more?
#define DEFAULT_MAX_TIME_BETWEEN_UPDATES												45000	//milisec A scanning cycle is about 32 sec
#define DEFAULT_TIMEOUT_FOR_REMOVING_BDADDRESS											(2 * 60000) // 2 minutes
#define DEFAULT_MAX_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING		10000	//milisec
#define DEFAULT_DESIRED_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING	3000	//milisec
#define DEFAULT_CLEANING_TIMEOUT_RESOLUTION												10000
#define DEFAULT_UPDATE_TIMEOUT_RESOLUTION												10000

static const char* CONFIG_SECTION = "PositioningParameters";

CPositioningAlgorithm::CPositioningAlgorithm(void) 
	: m_MinNumberOfParticipatingSensor(DEFAULT_MIN_NUMBER_OF_SENSORS), 
	m_DesiredNumberOfParticipatingSensor(DEFAULT_DESIRED_NUMBER_OF_SENSORS),
	m_MaxTimeBetweenUpdates(DEFAULT_MAX_TIME_BETWEEN_UPDATES),
	m_MaxTickCountDifferenceBetweenMeasuremnts(DEFAULT_MAX_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING),
	m_DesiredTickCountDifferenceBetweenMeasuremnts(DEFAULT_DESIRED_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING),
	m_TimeoutForRemovingBdaddress(DEFAULT_TIMEOUT_FOR_REMOVING_BDADDRESS),
	m_CleaningTimeoutResolution(DEFAULT_CLEANING_TIMEOUT_RESOLUTION),
	m_UpdateTimeoutResolution(DEFAULT_UPDATE_TIMEOUT_RESOLUTION),
	m_LastUpdateTimeoutTickCount(0),
	m_LastCleaningTimeoutTickCount(0),
	m_EstablishmentTopology(NULL)
{
}

CPositioningAlgorithm::~CPositioningAlgorithm(void)
{
}

bool CPositioningAlgorithm::Init()
{
	m_MinNumberOfParticipatingSensor = GetConfigInt(CONFIG_SECTION, "MinimumNumberOfSensorsForPositioning", DEFAULT_MIN_NUMBER_OF_SENSORS);
	m_DesiredNumberOfParticipatingSensor = GetConfigInt(CONFIG_SECTION, "DesiredNumberOfParticipatingSensor", DEFAULT_DESIRED_NUMBER_OF_SENSORS);
	m_MaxTimeBetweenUpdates = GetConfigInt(CONFIG_SECTION, "MaximumTimeBetweenUpdates ", DEFAULT_MAX_TIME_BETWEEN_UPDATES);
	m_TimeoutForRemovingBdaddress = GetConfigInt(CONFIG_SECTION, "TimeoutForRemovingBdaddress", DEFAULT_TIMEOUT_FOR_REMOVING_BDADDRESS);
	m_MaxTickCountDifferenceBetweenMeasuremnts = GetConfigInt(CONFIG_SECTION, "MaxTickCountDifferenceBetweenMeasuremnts", 
		DEFAULT_MAX_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING);
	m_DesiredTickCountDifferenceBetweenMeasuremnts = GetConfigInt(CONFIG_SECTION, "DesiredTickCountDifferenceBetweenMeasuremnts", 
		DEFAULT_DESIRED_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING);
	m_UpdateTimeoutResolution = GetConfigInt(CONFIG_SECTION, "UpdateTimeoutResolution", DEFAULT_UPDATE_TIMEOUT_RESOLUTION);
	m_CleaningTimeoutResolution = GetConfigInt(CONFIG_SECTION, "CleaningTimeoutResolution", DEFAULT_CLEANING_TIMEOUT_RESOLUTION);
	
	m_Impl = new CPositioningAlgorithmBasicImplementation();

	return m_Impl->Init();
}

void CPositioningAlgorithm::Close()
{
	delete m_Impl;
}

void CPositioningAlgorithm::Advise(
   CEstablishmentTopology* EstablishmentTopology, 
   IPositioningEvents *PositioningEventsHandler,
   IPositioningDebugReport *PositioningDebugReportHandler)
{
	m_PositioningEventsHandler = PositioningEventsHandler;
	m_EstablishmentTopology = EstablishmentTopology;
	m_Impl->AdviseEstablishmentTopology(EstablishmentTopology);
	m_Impl->AdviseDebugReport(PositioningDebugReportHandler);
}

void CPositioningAlgorithm::OnScannedData(const int &SensorId, const SScannedData& ScannedData)
{	
	// 1. update DB
	// 2. process data
	// 3. issue a Positioning Event if needed via IPositioningEvents interface

	m_ScannedBdAdressesDataBase.NewData(ScannedData.ScannedBDADDRESS, SensorId, ScannedData.RSSI, ScannedData.Time);

	DoPositioning(ScannedData.ScannedBDADDRESS, ScannedData.Time, m_DesiredNumberOfParticipatingSensor, m_DesiredTickCountDifferenceBetweenMeasuremnts);
}


void CPositioningAlgorithm::DoPositioning(std::string BdAddress, DWORD LastDataTickCount, DWORD NumberOfParticipatingSensor, DWORD TickCountDifferenceBetweenMeasuremnts)
{
	std::map<int /*SensorID*/, SMeasurement> Measurements = m_ScannedBdAdressesDataBase.GetScannedData(BdAddress, LastDataTickCount, TickCountDifferenceBetweenMeasuremnts);
	if (Measurements.size() < NumberOfParticipatingSensor)
		return;

	m_ScannedBdAdressesDataBase.BdAdressPositionUpdated(BdAddress);

	SPosition Accuracy;
	bool IsInEstablishment;
	SPosition EstimatedPosition = m_Impl->CalculatePosition(BdAddress, Measurements, Accuracy, IsInEstablishment);

		
	// Calc Geometric Average for Error \ Accuracy:
	double AccuracyRadii = sqrt(pow(Accuracy.x, 2) + pow(Accuracy.y, 2));

	// Calc Average Measurements Time for Positioning TimeStamp
	double PositioningTime = 0;
	for (unsigned int i = 0; i < Measurements.size(); i++)
		PositioningTime += Measurements[i].m_TickCount;

	PositioningTime /= Measurements.size();

	// Send Positioning Event
	m_PositioningEventsHandler->OnPositioning(BdAddress, EstimatedPosition, AccuracyRadii, PositioningTime, m_EstablishmentTopology->GetEstablishmentID(), IsInEstablishment);
}

void CPositioningAlgorithm::OnTimeout()
{
	DWORD Now = GetTickCount();
	
	if (Now - m_LastCleaningTimeoutTickCount > m_CleaningTimeoutResolution)
	{
		m_LastCleaningTimeoutTickCount = Now;
		m_ScannedBdAdressesDataBase.CleanDataBase(m_TimeoutForRemovingBdaddress);
	}
	

	if (Now - m_LastUpdateTimeoutTickCount > m_UpdateTimeoutResolution)
	{
		m_LastUpdateTimeoutTickCount = Now;
		PositionOutOfDateBdAddresses();
	}
}

void CPositioningAlgorithm::PositionOutOfDateBdAddresses()
{
	std::vector<std::string> BdAddressesToPosition = m_ScannedBdAdressesDataBase.GetBdAddressesToUpdate(m_MaxTimeBetweenUpdates);

	for (unsigned int i = 0; i < BdAddressesToPosition.size(); i++)
	{
		DWORD OldestLastTickCount = m_ScannedBdAdressesDataBase.GetOldestLastTickCount(BdAddressesToPosition[i], m_MaxTickCountDifferenceBetweenMeasuremnts);
		if (OldestLastTickCount == 0) // Can this happen?? probably yes...
		{
			LogEvent(LE_WARNING, __FUNCTION__ ": OldestLastTickCount returned 0 for BDADDRESS %s", BdAddressesToPosition[i].c_str());
			continue;
		}

		LogEvent(LE_INFO, __FUNCTION__ ": Positioning Out of date BDADDRESS %s.", BdAddressesToPosition[i].c_str());
		DoPositioning(BdAddressesToPosition[i], OldestLastTickCount, m_MinNumberOfParticipatingSensor, m_MaxTickCountDifferenceBetweenMeasuremnts);		
	}
}