#include "StdAfx.h"
#include "PositioningBasicAlgorithm.h"
#include "PositioningBasicAlgorithmManager.h"

#include "Matrix.h"
#include "Common/LogEvent.h"
#include "Common/collectionhelper.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CPositioningBasicAlgorithm::CPositioningBasicAlgorithm(void) : m_SensorsLocationMap(NULL) {}
CPositioningBasicAlgorithm::CPositioningBasicAlgorithm(ISensorsLocationMapInterface *SensorsLocationMap, SPosition InitialPosition) : m_SensorsLocationMap(SensorsLocationMap), m_LastPosition(InitialPosition) {}
CPositioningBasicAlgorithm::~CPositioningBasicAlgorithm(void) {}

#define GET_SENSOR_POSITION(SensorID, SensorPosition)\
if (!GetValueFromMap(*m_SensorsLocationMap->GetSensorsLocationMap(), SensorID, SensorPosition))	\
{																								\
	Assert(false);																				\
	LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d appears in Measurements yet does not appear in Sensors table !!", SensorID);\
	return CMatrix(); /*return EMPTY MATRIX to signal that a problem occurred*/					\
}

#define VERIFY_MATRIX(M)\
if (M.GetNumColumns() == 0 || M.GetNumRows() == 0)						\
{																		\
	Assert(M.GetNumColumns() == 0);										\
	Assert(M.GetNumRows() == 0);										\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to create %s Matrix", #M);\
	return InvalidPosition;												\
}

void CPositioningBasicAlgorithm::Init(ISensorsLocationMapInterface *TheSensorsLocationMap, SPosition InitialPosition)
{
	m_SensorsLocationMap = TheSensorsLocationMap;
	m_LastPosition = InitialPosition;
}

SPosition CPositioningBasicAlgorithm::CalcPosition(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts, SPosition &Accuracy, int &NumOfIterations)
{
	NumOfIterations = 0;
	DWORD StartTime = GetTickCount();
	SPosition EstimatedPosition = CalcPositionInternal(Measuremnts, Accuracy, NumOfIterations);
	double TotalTime = GetTickCount() - StartTime;
	
	if (EstimatedPosition == InvalidPosition)
		LogEvent(LE_ERROR, __FUNCTION__ "Failed to calculate Position. Iterations: %d, Time(seconds): %.02f", 
		NumOfIterations, TotalTime / 1000);
	else if (NumOfIterations >= m_SensorsLocationMap->GetMaxNumberOfIterations())
		LogEvent(LE_WARNING, __FUNCTION__ "Position Calculated: [%.02f,%.02f] with less accuracy. Used Maximum allowed Iterations: %d, Time(seconds): %.02f", 
			EstimatedPosition.x, EstimatedPosition.y, NumOfIterations, TotalTime / 1000);
	else
		LogEvent(LE_NOTICE, __FUNCTION__ "Position Calculated: [%.02f,%.02f] Iterations: %d, Time(seconds): %.02f", 
			EstimatedPosition.x, EstimatedPosition.y, NumOfIterations, TotalTime / 1000);

	return EstimatedPosition;
}

SPosition CPositioningBasicAlgorithm::CalcPositionInternal(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts, SPosition &Accuracy, int &NumOfIterations)
{
	NumOfIterations++;

	std::vector<int> ParticipatingSensors = GetListOfSensorsInMeasurements(Measuremnts);
	if ((int)Measuremnts.size() < m_SensorsLocationMap->GetMinNumberOfParticipatingSensor())
	{
		LogEvent(LE_WARNING, __FUNCTION__ ": Tried to calculate position with only %d Sensors. Minimum defined is %d. NOT CALCULATING!",
			Measuremnts.size(), m_SensorsLocationMap->GetMinNumberOfParticipatingSensor());
		return InvalidPosition;
	}

	CMatrix B = BuildMatrixB(ParticipatingSensors);
	VERIFY_MATRIX(B);

	CMatrix F = BuildMatrixF(Measuremnts);
	VERIFY_MATRIX(F);

	CMatrix Bt = B.GetTransposed();

	CMatrix Bsquared = (Bt * B);

	CMatrix Bsquared_inverted = Bsquared.GetInverted();

	CMatrix ErrorMatrix = Bsquared_inverted * Bt * F;

	double dX = ErrorMatrix[0][0];
	double dY = ErrorMatrix[1][0];

	UpdateLastLocation(dX, dY);

	if (IsPositionWellEstimated(dX, dY, NumOfIterations))
	{
		Accuracy.x = dX;
		Accuracy.y = dY;
		return m_LastPosition;
	}
	else
		return CalcPositionInternal(Measuremnts, Accuracy, NumOfIterations); /*Note: m_LastPosition has changed in this iteration !!*/
}

// Not every measurement must have data from all sensors. a Partial data (from some of the sensors)
// is also valid. But, in order to use algorithm appropriately, sensors participating in a certain 
// measurement must be identified.
std::vector<int> CPositioningBasicAlgorithm::GetListOfSensorsInMeasurements(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts) const
{
	std::vector<int> ParticipatingSensors;

	std::map<int /*SensorID*/, double>::iterator Iter = Measuremnts.begin();
	std::map<int /*SensorID*/, double>::iterator End = Measuremnts.end();

	for(;Iter != End; ++Iter)
		ParticipatingSensors.push_back(Iter->first);

	return ParticipatingSensors;
}

CMatrix CPositioningBasicAlgorithm::BuildMatrixB(std::vector<int> ParticipatingSensors) const
{
	Assert(ParticipatingSensors.size() <= m_SensorsLocationMap->GetSensorsLocationMap()->size());
	Assert((int)ParticipatingSensors.size() >= m_SensorsLocationMap->GetMinNumberOfParticipatingSensor());

	CMatrix B((int)ParticipatingSensors.size(), (int)2);

	for (unsigned int i = 0; i < ParticipatingSensors.size(); i++)
	{
		SPosition SensorPosition;
		GET_SENSOR_POSITION(ParticipatingSensors[i], SensorPosition);

		B[i][0] = CalcBCellX(SensorPosition);
		B[i][1] = CalcBCellY(SensorPosition);
	}

	return B;
}

double CPositioningBasicAlgorithm::CalcBCellX(SPosition SensorPosition) const
{
	double X1 = SensorPosition.x;
	double Xe = m_LastPosition.x;

	double Distance = CalcDistanceFromSensor(SensorPosition);
	double Value = (X1 - Xe) / Distance;

	return Value;
}

double CPositioningBasicAlgorithm::CalcBCellY(SPosition SensorPosition) const
{
	double Y1 = SensorPosition.y;
	double Ye = m_LastPosition.y;

	double Distance = CalcDistanceFromSensor(SensorPosition);
	double Value = (Y1 - Ye) / Distance;

	return Value;
}


double CPositioningBasicAlgorithm::CalcDistanceFromSensor(SPosition SensorPosition) const
{
	return CalcDistance(SensorPosition.x, SensorPosition.y, m_LastPosition.x, m_LastPosition.y);
}

double CPositioningBasicAlgorithm::CalcDistance(double X1, double Y1, double Xe, double Ye)
{
	return sqrt(pow(X1 - Xe, 2) + pow(Y1 - Ye, 2));
}

CMatrix CPositioningBasicAlgorithm::BuildMatrixF(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts) const
{
	std::map<int /*SensorID*/, double>::iterator Iter = Measuremnts.begin();
	std::map<int /*SensorID*/, double>::iterator End = Measuremnts.end();

	CMatrix F((int)Measuremnts.size(), (int)1);

	for(int i = 0;Iter != End; ++Iter, i++)
	{
		int SensorID = Iter->first;
		double MeasuredDistance = Iter->second;

		SPosition SensorPosition;
		GET_SENSOR_POSITION(SensorID, SensorPosition);

		F[i][0] = CalcFCell(SensorPosition, MeasuredDistance);
	}

	return F;
}


double CPositioningBasicAlgorithm::CalcFCell(SPosition SensorPosition, double MeasuredDistance) const
{
	double Distance = CalcDistanceFromSensor(SensorPosition);
	double F = Distance - MeasuredDistance;
	return F;
}

void CPositioningBasicAlgorithm::UpdateLastLocation(double dX, double dY)
{
	m_LastPosition.x += dX;
	m_LastPosition.y += dY;
}

bool CPositioningBasicAlgorithm::IsPositionWellEstimated(double dX, double dY, int NumOfIterations) const
{
	double MaxError = m_SensorsLocationMap->GetMaxAcceptablePositioningError();
	double MaxIterations = m_SensorsLocationMap->GetMaxNumberOfIterations();
	return ((abs(dX) <= MaxError && abs(dY) <= MaxError) || NumOfIterations >= MaxIterations);
}