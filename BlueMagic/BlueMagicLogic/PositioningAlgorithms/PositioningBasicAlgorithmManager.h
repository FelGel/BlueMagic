#pragma once

#include <map>
#include "PositioningBasicAlgorithm.h"
#include "PositioningAlgorithms.h"


class ISensorsLocationMapInterface
{
public:
	virtual std::map<int /*SensorID*/, SPosition> *GetSensorsLocationMap() = 0;
	virtual double GetMaxAcceptablePositioningError() const = 0;
	virtual double GetMaxNumberOfIterations() const = 0;
	virtual int GetMinNumberOfParticipatingSensor() const = 0;
};

class POSITIONINGALGORITHMS_API CPositioningBasicAlgorithmManager : public ISensorsLocationMapInterface
{
public:
	CPositioningBasicAlgorithmManager(void);
	~CPositioningBasicAlgorithmManager(void);
	
	void Init(std::map<int /*SensorID*/, SPosition> TheSensorsLocationMap, 
		SPosition InitialPosition, 
		double MaxAcceptablePositioningError, 
		double MaxNumberOfIterations,
		int MinNumberOfParticipatingSensor);

	SPosition CalcPosition(std::string BDADDRESS, std::map<int /*SensorID*/, double /*Distance*/> Measuremnts);

	virtual std::map<int /*SensorID*/, SPosition> *GetSensorsLocationMap();
	virtual double GetMaxAcceptablePositioningError() const;
	virtual int GetMinNumberOfParticipatingSensor() const;
	virtual double GetMaxNumberOfIterations() const;

private:
	std::map<std::string /*BDADDRESS*/, CPositioningBasicAlgorithm> m_DetectedBDaddresses;

	std::map<int /*SensorID*/, SPosition> m_TheSensorsLocationMap;
	SPosition m_InitialPosition;

	double m_MaxAcceptablePositioningError;
	double m_MaxNumberOfIterations;
	int m_MinNumberOfParticipatingSensor;
};
