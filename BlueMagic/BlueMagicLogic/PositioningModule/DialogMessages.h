#pragma once

#include "ISensorEvents.h"

enum EDialogMessageType
{
	DialogDataMessage,
	DialogSensorMessage,
	DialogPositioningMessage,
	DialogEstablishmentContourMessage,
	DialogSensorsLocationMessage
};

struct SDialogMessage
{
	SDialogMessage(EDialogMessageType MessageType, int SensorId) : m_MessageType(MessageType), m_SensorId(SensorId) {}
	virtual ~SDialogMessage() {}
	
	EDialogMessageType m_MessageType;
	int	m_SensorId;
};

struct SDialogDataMessage : public SDialogMessage
{
	SDialogDataMessage(int SensorId, SScannedData ScannedData, CString TimeStamp) 
		: SDialogMessage(DialogDataMessage, SensorId), m_ScannedData(ScannedData), m_TimeStamp(TimeStamp) {}
	virtual ~SDialogDataMessage() {}

	SScannedData	m_ScannedData;
	CString			m_TimeStamp;
};


struct SDialogSensorMessage : public SDialogMessage
{
	SDialogSensorMessage(const int &SensorId, const bool &IsController, const ESensorConnectionStatus &SensorConnectionStatus, const ESensorHandshakeStatus &SensorHandshakeStatus, const ESensorActivityStatus &SensorActivityStatus)
		: SDialogMessage(DialogSensorMessage, SensorId), m_IsController(IsController), m_SensorConnectionStatus(SensorConnectionStatus),
			m_SensorHandshakeStatus(SensorHandshakeStatus), m_SensorActivityStatus(SensorActivityStatus) {}
	virtual ~SDialogSensorMessage() {}

	bool m_IsController;
	ESensorConnectionStatus m_SensorConnectionStatus;
	ESensorHandshakeStatus m_SensorHandshakeStatus;
	ESensorActivityStatus m_SensorActivityStatus;
};

struct SDialogPositioingMessage : public SDialogMessage
{
	SDialogPositioingMessage(
		std::string BDADDRESS, 
		std::map<int /*SensorID*/, SMeasurement> Measurements,
		std::map<int /*SensorID*/, double /*SmoothedDistance*/> DistanceEstimations,
		SPosition EstimatedPosition,
		SPosition EstimatedPositionError,
		int NumOfIterations, bool IsInEstablishment) : SDialogMessage(DialogPositioningMessage, 0), 
		m_BDADDRESS(BDADDRESS), m_Measurements(Measurements),
		m_DistanceEstimations(DistanceEstimations), m_EstimatedPosition(EstimatedPosition),
		m_EstimatedPositionError(EstimatedPositionError), m_NumOfIterations(NumOfIterations),
		m_IsInEstablishment (IsInEstablishment) {}
	virtual ~SDialogPositioingMessage() {}

	std::string m_BDADDRESS; 
	std::map<int /*SensorID*/, SMeasurement> m_Measurements;
	std::map<int /*SensorID*/, double /*SmoothedDistance*/> m_DistanceEstimations;
	SPosition m_EstimatedPosition;
	SPosition m_EstimatedPositionError;
	int m_NumOfIterations;
	bool m_IsInEstablishment;
};


struct SDialogEstablishmentContourMessage : public SDialogMessage
{
	SDialogEstablishmentContourMessage(
		std::vector<SPosition> EstablishmentCoordinates, 
		std::vector<SDepartmentInfo> DepartmentsInfo) : SDialogMessage(DialogEstablishmentContourMessage, 0),
		m_EstablishmentCoordinates(EstablishmentCoordinates),
		m_DepartmentsInfo(DepartmentsInfo) {}

	virtual ~SDialogEstablishmentContourMessage() {}

	std::vector<SPosition> m_EstablishmentCoordinates;
	std::vector<SDepartmentInfo> m_DepartmentsInfo;
};

struct SDialogSensorsLocationMessage : public SDialogMessage
{
	SDialogSensorsLocationMessage(
		std::map<int /*SensorID*/, SPosition> SensorsLocation) : SDialogMessage(DialogSensorsLocationMessage, 0),
		m_SensorsLocation(SensorsLocation) {}
	virtual ~SDialogSensorsLocationMessage() {}

	std::map<int /*SensorID*/, SPosition> m_SensorsLocation;
};


class IDialogMessagesInterface
{
public:
	virtual void SendMessageToDialog(SDialogDataMessage *Message) = 0;
	virtual void SendMessageToDialog(SDialogSensorMessage *Message) = 0;
	virtual void SendMessageToDialog(SDialogPositioingMessage *Message) = 0;
	virtual void SendMessageToDialog(SDialogEstablishmentContourMessage *Message) = 0;
	virtual void SendMessageToDialog(SDialogSensorsLocationMessage *Message) = 0;

	virtual void OnTimeoutCalledFromPositioningManager() = 0;
};

