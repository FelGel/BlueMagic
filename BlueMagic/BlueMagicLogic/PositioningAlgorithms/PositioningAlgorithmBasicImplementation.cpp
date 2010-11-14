#include "StdAfx.h"
#include "PositioningAlgorithmBasicImplementation.h"
#include "Common/LogEvent.h"
#include "Common/Config.h"
#include "Common/Utils.h"
#include "Common/collectionhelper.h"

const char* SensorsConfigSection	= "SensorsPositioningParameters";
const char* Prefix					= "Sensor";
const char* GeneralConfigSection	= "PositioningParameters";

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define DEAFULT_MAX_ACCEPTABLE_POSITIONING_ERROR	1.0		// meters
#define DEAFULT_MAX_NUMBER_OF_ITERATIONS			100	// iterations
#define DEFAULT_MIN_NUMBER_OF_SENSORS				3		// sensors

#define VALIDATE_PARAMETER(p)\
if (p == ILLEGAL_##p)		\
{							\
	LogEvent(LE_ERROR, __FUNCTION__ ": illegal %s value (%f) for SensorID %d",\
		#p, p, SensorID);	\
	continue;				\
}

#define ADD_PARAMS_TO_MAP(SensorID, Param, Map)\
if (!InsertValueToMap(Map, SensorID, Param))	\
{												\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to add SensorID %d %s to map. Do you have Duplicate SensorIDs in map?",\
		SensorID, #Param);						\
	return false;								\
}


CPositioningAlgorithmBasicImplementation::CPositioningAlgorithmBasicImplementation(void)
	: m_DebugReportHandler(NULL), m_EstablishmentTopology(NULL)
{
}

CPositioningAlgorithmBasicImplementation::~CPositioningAlgorithmBasicImplementation(void)
{
}

/*virtual*/ bool CPositioningAlgorithmBasicImplementation::Init()
{
	std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams> SensorsDistanceParams;
	std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams> SensorsSmoothingParams;
	std::map<int /*SensorID*/, SPosition> SensorsLocationParams;

	if (!ReadSensorsConfiguration(SensorsDistanceParams, SensorsSmoothingParams, SensorsLocationParams))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Read Sensors Positioning Parameters Configuration");
		return false;
	}
	SendSensorsLocationReport(SensorsLocationParams);

	if (!m_DistanceAlgorithm.Init(SensorsDistanceParams))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Init Distance Estimation Algorithm");
		return false;
	}

	if (!m_SmoothingAlgorithm.Init(SensorsSmoothingParams))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Init Distance Smoothing Algorithm");
		return false;
	}

	SPosition InitialPosition;
	InitialPosition.x = GetConfigDouble(GeneralConfigSection, "InitialPosition_X ", InvalidPosition.x);
	InitialPosition.y = GetConfigDouble(GeneralConfigSection, "InitialPosition_Y ", InvalidPosition.y);

	if (InitialPosition.x == InvalidPosition.x || InitialPosition.y == InvalidPosition.y)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": InitialPosition is not properly configured!, x = %f, y = %f",
			InitialPosition.x, InitialPosition.y);
		return false;
	}

	double MaxAcceptablePositioningError = GetConfigDouble(GeneralConfigSection, "MaxAcceptablePositioningError ", DEAFULT_MAX_ACCEPTABLE_POSITIONING_ERROR);
	int MaxNumberOfIterations = GetConfigInt(GeneralConfigSection, "MaxNumberOfIterations ", DEAFULT_MAX_NUMBER_OF_ITERATIONS);
	int MinNumberOfParticipatingSensor = GetConfigInt(GeneralConfigSection, "MinNumberOfParticipatingSensor ", DEFAULT_MIN_NUMBER_OF_SENSORS);

	if (!VerifyParameters(MaxAcceptablePositioningError, MaxNumberOfIterations, MinNumberOfParticipatingSensor))
		return false; // No need for log. VerifyParameters takes care of that.

	m_PositioningAlgorithm.Init(SensorsLocationParams, InitialPosition, MaxAcceptablePositioningError, MaxNumberOfIterations, MinNumberOfParticipatingSensor);

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Positioning Algorithm Initialized successfully! InitialPosition=[%f,%f], MaxAcceptablePositioningError=%f, MaxNumberOfIterations=%d, MinNumberOfParticipatingSensor=%d",
		InitialPosition.x, InitialPosition.y, MaxAcceptablePositioningError, MaxNumberOfIterations, MinNumberOfParticipatingSensor);

	return true;
}

/*virtual*/ void CPositioningAlgorithmBasicImplementation::AdviseEstablishmentTopology(CEstablishmentTopology *EstablishmentTopology)
{
	m_EstablishmentTopology = EstablishmentTopology;
}

/*virtual*/ SPosition CPositioningAlgorithmBasicImplementation::CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts, SPosition &Accuracy, bool &IsInEstablishment)
{
	// Rssi Measurements
	DumpRssiMeasurementsToLog(BDADDRESS, Measuremnts);

	// Distance Estimations
	std::map<int /*SensorID*/, double /*Distance*/> DistanceEstimations;
	EstimateDistances(BDADDRESS, Measuremnts, DistanceEstimations);
	DumpDistanceEstimationsToLog(BDADDRESS, DistanceEstimations);

	// Position Estimation
	int NumOfIterations;
	SPosition EstimatedPosition = m_PositioningAlgorithm.CalcPosition(BDADDRESS, DistanceEstimations, Accuracy, NumOfIterations);
	DumpEstimatedPositionToLog(BDADDRESS, EstimatedPosition);

	IsInEstablishment = m_EstablishmentTopology->IsMeasurementInEstablishemnt(EstimatedPosition);

	SendDebugReport(BDADDRESS, Measuremnts, DistanceEstimations, EstimatedPosition, Accuracy, NumOfIterations, IsInEstablishment);
	return EstimatedPosition;
}

void CPositioningAlgorithmBasicImplementation::EstimateDistances(
	std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts,
	std::map<int /*SensorID*/, double /*Distance*/> &DistanceEstimations)
{
	std::map<int /*SensorID*/, SMeasurement>::iterator Iter = Measuremnts.begin();
	std::map<int /*SensorID*/, SMeasurement>::iterator End = Measuremnts.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		SMeasurement &Measurement = Iter->second;

		double Distance = m_DistanceAlgorithm.CalcDistance(SensorID, Measurement.m_RSSI);
		double SmoothedDistance = m_SmoothingAlgorithm.SmoothDistance(SensorID, BDADDRESS, Distance, Measurement.m_TickCount);

		if (!InsertValueToMap(DistanceEstimations, SensorID, SmoothedDistance))
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Failed to insert SmoothedDistance to DistanceEstimations map for SensorID %d", SensorID);
			continue;
		}
	}
}

void CPositioningAlgorithmBasicImplementation::DumpRssiMeasurementsToLog(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts)
{
	std::string PositionDataString;

	std::map<int /*SensorID*/, SMeasurement>::iterator Iter = Measuremnts.begin();
	std::map<int /*SensorID*/, SMeasurement>::iterator End = Measuremnts.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		SMeasurement &Measurement = Iter->second;

		CString MeasurementString;
		MeasurementString.Format("ID: %d, RSSI: %d. ", SensorID, Measurement.m_RSSI);

		PositionDataString += MeasurementString;
	}

	LogEvent(LE_NOTICE, __FUNCTION__ ": RSSI Measurements for BDADDRESS %s. Data: %s", 
		BDADDRESS.c_str(), PositionDataString.c_str());
}

void CPositioningAlgorithmBasicImplementation::DumpEstimatedPositionToLog(std::string BDADDRESS, SPosition EstimatedPosition)
{
	LogEvent(LE_NOTICE, __FUNCTION__ ": Estimated Position for BDADDRESS %s: [%f,%f]", 
		BDADDRESS.c_str(), EstimatedPosition.x, EstimatedPosition.y);
}

void CPositioningAlgorithmBasicImplementation::DumpDistanceEstimationsToLog(std::string BDADDRESS, std::map<int /*SensorID*/, double /*Distance*/> DistanceEstimations)
{
	std::string PositionDataString;

	std::map<int /*SensorID*/, double /*Distance*/>::iterator Iter = DistanceEstimations.begin();
	std::map<int /*SensorID*/, double /*Distance*/>::iterator End = DistanceEstimations.end();

	for(;Iter != End; ++Iter)
	{
		int SensorID = Iter->first;
		double &Distance = Iter->second;

		CString MeasurementString;
		MeasurementString.Format("ID: %d, Distance: %f. ", SensorID, Distance);

		PositionDataString += MeasurementString;
	}

	LogEvent(LE_NOTICE, __FUNCTION__ ": Distance Estimations for BDADDRESS %s. Data: %s", 
		BDADDRESS.c_str(), PositionDataString.c_str());
}

bool CPositioningAlgorithmBasicImplementation::ReadSensorsConfiguration(
	std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams> &SensorsDistanceParams,
	std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams> &SensorsSmoothingParams,
	std::map<int /*SensorID*/, SPosition> &SensorsLocationParams)
{
	std::vector<ConfigListItem> ObjectSections;
	GetListSection(SensorsConfigSection, Prefix, ObjectSections);
	//////////////////////////////////////////////////////////////////////////

	if (ObjectSections.size() == 0)
	{
		LogEvent(LE_ERROR, "Failed to read Sensors' parameters from Configuration !");
		return false;
	}

	for (int i = 0; (unsigned)i < ObjectSections.size(); ++i) 
	{
		std::string ObjectSection = ObjectSections[i].ItemValue;

		// Read: SensorID, A, N for each Sensor:
		int SensorID = GetIntValue(ObjectSection.c_str(), "SensorID=", -1);//GetConfigSectionId(ConfigSection);   
		double A = GetDoubleValue(ObjectSection.c_str(), "A=", ILLEGAL_A);
		double N = GetDoubleValue(ObjectSection.c_str(), "N=", ILLEGAL_N);
		double a = GetDoubleValue(ObjectSection.c_str(), "a=", ILLEGAL_a);
		double b = GetDoubleValue(ObjectSection.c_str(), "b=", ILLEGAL_b);
		double x = GetDoubleValue(ObjectSection.c_str(), "x=", ILLEGAL_x);
		double y = GetDoubleValue(ObjectSection.c_str(), "y=", ILLEGAL_y);

		if (SensorID <= 0)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": illegal SensorID %d", SensorID);
			continue;
		}

		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Position Algorithm parameters read for Sensor %d: a=%f,b=%f,A=%f,N=%f,x=%f,y=%f", 
			SensorID, a,b,A,N,x,y);

		VALIDATE_PARAMETER(A);
		VALIDATE_PARAMETER(N);
		VALIDATE_PARAMETER(a);
		VALIDATE_PARAMETER(b);
		VALIDATE_PARAMETER(x);
		VALIDATE_PARAMETER(y);

		SRssiToDistanceBasicAlgorithmParams DistanceParams	(A, N);
		SDistanceSmoothingAlgorithmParams	SmoothingParams	(a, b);
		SPosition							LocationParams	(x, y);


		ADD_PARAMS_TO_MAP(SensorID, DistanceParams, SensorsDistanceParams);
		ADD_PARAMS_TO_MAP(SensorID, SmoothingParams, SensorsSmoothingParams);
		ADD_PARAMS_TO_MAP(SensorID, LocationParams, SensorsLocationParams);
	}

	Assert(SensorsDistanceParams.size() == SensorsSmoothingParams.size()
		&& SensorsDistanceParams.size() == SensorsLocationParams.size());

	if (SensorsDistanceParams.size() == 0)
	{
		LogEvent(LE_WARNING, __FUNCTION__ ": No Sensor Positioning Algorithm Parameters were read");
		return false;
	}

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Positioning Algorithm Parameters were read for %d sensors", SensorsDistanceParams.size());
	return true;
}

// ToDo: check params also for "reasonable" values
#define MAX_EXPECTED_POSITION_ERROR			10.0
#define MIN_EXPECTED_POSITION_ERROR			0.1
#define MIN_EXPECTED_NUMBER_OF_ITERATIONS	100
#define MAX_EXPECTED_NUMBER_OF_ITERATIONS	1000000
#define MIN_EXPECTED_NUMBER_OF_SENSORS		3
#define MAX_EXPECTED_NUMBER_OF_SENSORS		10

bool CPositioningAlgorithmBasicImplementation::VerifyParameters(double MaxAcceptablePositioningError, int MaxNumberOfIterations, int MinNumberOfParticipatingSensor) const
{
	Assert(MaxAcceptablePositioningError > 0);
	Assert(MaxNumberOfIterations > 0);
	Assert(MinNumberOfParticipatingSensor > 0);

	if (MaxAcceptablePositioningError <= 0)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": MaxAcceptablePositioningError = %f smaller than 0 !!", 
			MaxAcceptablePositioningError);
		return false;
	}

	if (MaxNumberOfIterations <= 0)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": MaxNumberOfIterations = %d smaller than 0 !!", 
			MaxNumberOfIterations);
		return false;
	}

	if (MinNumberOfParticipatingSensor <= 0)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": MinNumberOfParticipatingSensor = %d smaller than 0 !!", 
			MinNumberOfParticipatingSensor);
		return false;
	}

	return true;
}

/*virtual */void CPositioningAlgorithmBasicImplementation::AdviseDebugReport(IPositioningDebugReport *DebugReportHandler)
{
	m_DebugReportHandler = DebugReportHandler;
	m_DebugReportHandler->OnSensorsLocationReport(*m_PositioningAlgorithm.GetSensorsLocationMap());
}

void CPositioningAlgorithmBasicImplementation::SendDebugReport(
	std::string BDADDRESS, 
	std::map<int /*SensorID*/, SMeasurement> Measurements,
	std::map<int /*SensorID*/, double /*SmoothedDistance*/> DistanceEstimations,
	SPosition EstimatedPosition,
	SPosition EstimatedPositionError,
	int NumOfIterations, 
	bool IsInEstablishment)
{
	if (m_DebugReportHandler)
	{
		m_DebugReportHandler->OnPositioningDebugReport(
			BDADDRESS, 
			Measurements, 
			DistanceEstimations, 
			EstimatedPosition, 
			EstimatedPositionError, 
			NumOfIterations, 
			IsInEstablishment);
	}
}


void CPositioningAlgorithmBasicImplementation::SendSensorsLocationReport(std::map<int /*SensorID*/, SPosition> SensorsLocation)
{
	if (m_DebugReportHandler)
		m_DebugReportHandler->OnSensorsLocationReport(SensorsLocation);
}