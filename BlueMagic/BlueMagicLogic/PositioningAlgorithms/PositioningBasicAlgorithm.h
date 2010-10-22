#pragma once

#include "PositionStructure.h"
#include "PositioningAlgorithms.h"

class ISensorsLocationMapInterface;
class CMatrix;

#include <map>
#include <vector>

class POSITIONINGALGORITHMS_API CPositioningBasicAlgorithm
{
public:
	CPositioningBasicAlgorithm(void);
	CPositioningBasicAlgorithm(ISensorsLocationMapInterface *SensorsLocationMap, SPosition InitialPosition);
	~CPositioningBasicAlgorithm(void);

	void Init(ISensorsLocationMapInterface *TheSensorsLocationMap, SPosition InitialPosition);
	SPosition CalcPosition(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts);

private:
	std::vector<int> GetListOfSensorsInMeasurements(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts) const;
	SPosition CalcPositionInternal(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts, int &NumOfIterations);
	CMatrix BuildMatrixB(std::vector<int> ParticipatingSensors) const;
	CMatrix BuildMatrixF(std::map<int /*SensorID*/, double /*Distance*/> Measuremnts) const;
	double CalcBCellX(SPosition SensorPosition) const;
	double CalcBCellY(SPosition SensorPosition) const;
	double CalcFCell(SPosition SensorPosition, double MeasuredDistance) const;
	double CalcDistanceFromSensor(SPosition SensorPosition) const;
	double static CalcDistance(double X1, double Y1, double Xe, double Ye);
	void UpdateLastLocation(double dX, double dY);
	bool IsPositionWellEstimated(double dX, double dY, int NumOfIterations) const;

private:	
	ISensorsLocationMapInterface *m_SensorsLocationMap;
	SPosition m_LastPosition;
};
