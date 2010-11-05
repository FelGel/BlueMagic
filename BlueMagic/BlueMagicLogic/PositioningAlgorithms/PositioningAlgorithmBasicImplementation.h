#pragma once

#include <map>
#include "PositioningAlgorithms.h"
#include "IPositioningAlgorithmImplementation.h"

#include "RssiToDistanceBasicAlgorithmManager.h"
#include "DistanceSmoothingBasicAlgorithmGeneralManager.h"
#include "PositioningBasicAlgorithmManager.h"


class POSITIONINGALGORITHMS_API CPositioningAlgorithmBasicImplementation :
	public IPositioningAlgorithmImplementation
{
public:
	CPositioningAlgorithmBasicImplementation(void);
	~CPositioningAlgorithmBasicImplementation(void);

	virtual bool Init();
	virtual SPosition CalculatePosition(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts, SPosition &Accuracy);
	virtual void AdviseDebugReport(IPositioningDebugReport *DebugReportHandler);

private:
	void DumpRssiMeasurementsToLog(std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts);
	void DumpDistanceEstimationsToLog(std::string BDADDRESS, std::map<int /*SensorID*/, double /*Distance*/> DistanceEstimations);
	void DumpEstimatedPositionToLog(std::string BDADDRESS, SPosition EstimatedPosition);
	bool ReadSensorsConfiguration(
		std::map<int /*SensorID*/, SRssiToDistanceBasicAlgorithmParams> &SensorsDistanceParams,
		std::map<int /*SensorID*/, SDistanceSmoothingAlgorithmParams> &SensorsSmoothingParams,
		std::map<int /*SensorID*/, SPosition> &SensorsLocationParams);
	void EstimateDistances(
		std::string BDADDRESS, std::map<int /*SensorID*/, SMeasurement> Measuremnts,
		std::map<int /*SensorID*/, double /*Distance*/> &DistanceEstimations);
	bool VerifyParameters(double MaxAcceptablePositioningError, int MaxNumberOfIterations, int MinNumberOfParticipatingSensor) const;
	void SendDebugReport(
		std::string BDADDRESS, 
		std::map<int /*SensorID*/, SMeasurement> Measurements,
		std::map<int /*SensorID*/, double /*SmoothedDistance*/> DistanceEstimations,
		SPosition EstimatedPosition,
		SPosition EstimatedPositionError,
		int NumOfIterations);

	void SendSensorsLocationReport(std::map<int /*SensorID*/, SPosition> SensorsLocation);

private:
	CRssiToDistanceBasicAlgorithmManager			m_DistanceAlgorithm;
	CDistanceSmoothingBasicAlgorithmGeneralManager	m_SmoothingAlgorithm;
	CPositioningBasicAlgorithmManager				m_PositioningAlgorithm;

	IPositioningDebugReport *m_DebugReportHandler;
};
