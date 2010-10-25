#include "StdAfx.h"
#include "PositioningAlgorithm.h"

#include "Common/collectionhelper.h"
#include "Common/LogEvent.h"
#include "Common/Config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEFAULT_MIN_NUMBER_OF_SENSORS													3		//sensors
#define DEFAULT_DESIRED_NUMBER_OF_SENSORS												3		//sensors
#define DEFAULT_MAX_TIME_BETWEEN_UPDATES												9000	//milisec
#define DEFAULT_MAX_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING		3000	//milisec
#define DEFAULT_DESIRED_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING	10000	//milisec
#define DEFAULT_TIMEOUT_FOR_REMOVING_BDADDRESS											(2 * 60000) // 2 minutes

static const char* CONFIG_SECTION = "PositioningParameters";

CPositioningAlgorithm::CPositioningAlgorithm(void) 
	: m_MinNumberOfParticipatingSensor(DEFAULT_MIN_NUMBER_OF_SENSORS), 
	m_DesiredNumberOfParticipatingSensor(DEFAULT_DESIRED_NUMBER_OF_SENSORS),
	m_MaxTimeBetweenUpdates(DEFAULT_MAX_TIME_BETWEEN_UPDATES),
	m_MaxTickCountDifferenceBetweenMeasuremnts(DEFAULT_MAX_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING),
	m_DesiredTickCountDifferenceBetweenMeasuremnts(DEFAULT_DESIRED_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING),
	m_TimeoutForRemovingBdaddress(DEFAULT_TIMEOUT_FOR_REMOVING_BDADDRESS)
{
}

CPositioningAlgorithm::~CPositioningAlgorithm(void)
{
}

void CPositioningAlgorithm::Init()
{
	m_MinNumberOfParticipatingSensor = GetConfigInt(CONFIG_SECTION, "MinimumNumberOfSensorsForPositioning", DEFAULT_MIN_NUMBER_OF_SENSORS);
	m_DesiredNumberOfParticipatingSensor = GetConfigInt(CONFIG_SECTION, "DesiredNumberOfParticipatingSensor", DEFAULT_DESIRED_NUMBER_OF_SENSORS);
	m_MaxTimeBetweenUpdates = GetConfigInt(CONFIG_SECTION, "MaximumTimeBetweenUpdates ", DEFAULT_MAX_TIME_BETWEEN_UPDATES);
	m_TimeoutForRemovingBdaddress = GetConfigInt(CONFIG_SECTION, "TimeoutForRemovingBdaddress", DEFAULT_TIMEOUT_FOR_REMOVING_BDADDRESS);
	m_MaxTickCountDifferenceBetweenMeasuremnts = GetConfigInt(CONFIG_SECTION, "MaxTickCountDifferenceBetweenMeasuremnts", 
		DEFAULT_MAX_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING);
	m_DesiredTickCountDifferenceBetweenMeasuremnts = GetConfigInt(CONFIG_SECTION, "DesiredTickCountDifferenceBetweenMeasuremnts", 
		DEFAULT_DESIRED_TICKCOUNT_DIFFERENCE_BETWEEN_MEASUREMNTS_OF_SAME_POSITIONING);
}

void CPositioningAlgorithm::Advise(CEstablishmentTopology* EstablishmentTopology, IPositioningEvents *PositioningEventsHandler)
{
	m_EstablishmentTopology = EstablishmentTopology;
	m_PositioningEventsHandler = PositioningEventsHandler;
}

void CPositioningAlgorithm::OnScannedData(const int &SensorId, const SScannedData& ScannedData)
{	
	// 1. update DB
	// 2. process data
	// 3. issue a Positioning Event if needed via IPositioningEvents interface

	m_ScannedBdAdressesDataBase.NewData(ScannedData.ScannedBDADDRESS, SensorId, ScannedData.RSSI, ScannedData.Time);

	DoPositioning(ScannedData.ScannedBDADDRESS, m_DesiredNumberOfParticipatingSensor, m_DesiredTickCountDifferenceBetweenMeasuremnts);
}


void CPositioningAlgorithm::DoPositioning(std::string BdAddress, DWORD NumberOfParticipatingSensor, DWORD TickCountDifferenceBetweenMeasuremnts)
{
	std::map<int /*SensorID*/, SMeasurement> Measurements = m_ScannedBdAdressesDataBase.GetScannedData(BdAddress, TickCountDifferenceBetweenMeasuremnts);
	if (Measurements.size() < NumberOfParticipatingSensor)
		return;

	m_ScannedBdAdressesDataBase.BdAdressPositionUpdated(BdAddress);

	SPosition EstimatedPosition = m_Impl->CalculatePosition(BdAddress, Measurements);

	// ToDo:
	// 1. calculate the TickCount of the positioning (average of all measurements)
	// 2. Use another algorithm IsInEstablishment
	// 3. Accuracy - should be calculated  (HOW??) !!

	m_PositioningEventsHandler->OnPositioning(BdAddress, EstimatedPosition, -1 /*?*/, 0, m_EstablishmentTopology->GetEstablishmentID(), false /*UseAnotherAlgorithm*/);
}

void CPositioningAlgorithm::OnTimeout()
{
	m_ScannedBdAdressesDataBase.CleanDataBase(m_TimeoutForRemovingBdaddress);

	PositionOutOfDateBdAddresses();
}

void CPositioningAlgorithm::PositionOutOfDateBdAddresses()
{
	std::vector<std::string> BdAddressesToPosition = m_ScannedBdAdressesDataBase.GetBdAddressesToUpdate(m_MaxTimeBetweenUpdates);

	for (unsigned int i = 0; i < BdAddressesToPosition.size(); i++)
	{
		DoPositioning(BdAddressesToPosition[i], m_MinNumberOfParticipatingSensor, m_MaxTickCountDifferenceBetweenMeasuremnts);		
	}
}