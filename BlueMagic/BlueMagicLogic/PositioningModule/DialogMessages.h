#pragma once

#include "ISensorEvents.h"

enum EDialogMessageType
{
	DialogDataMessage,
	DialogSensorMessage
};

struct SDialogMessage
{
	SDialogMessage(EDialogMessageType MessageType, int SensorId) : m_MessageType(MessageType), m_SensorId(SensorId) {}
	
	EDialogMessageType m_MessageType;
	int	m_SensorId;
};

struct SDialogDataMessage : public SDialogMessage
{
	SDialogDataMessage(int SensorId, SScannedData ScannedData, CString TimeStamp) 
		: SDialogMessage(DialogDataMessage, SensorId), m_ScannedData(ScannedData), m_TimeStamp(TimeStamp) {}

	SScannedData	m_ScannedData;
	CString			m_TimeStamp;
};


struct SDialogSensorMessage : public SDialogMessage
{
	SDialogSensorMessage(const int &SensorId, const bool &IsController, const ESensorConnectionStatus &SensorConnectionStatus, const ESensorHandshakeStatus &SensorHandshakeStatus, const ESensorActivityStatus &SensorActivityStatus)
		: SDialogMessage(DialogSensorMessage, SensorId), m_IsController(IsController), m_SensorConnectionStatus(SensorConnectionStatus),
			m_SensorHandshakeStatus(SensorHandshakeStatus), m_SensorActivityStatus(SensorActivityStatus) {}

	bool m_IsController;
	ESensorConnectionStatus m_SensorConnectionStatus;
	ESensorHandshakeStatus m_SensorHandshakeStatus;
	ESensorActivityStatus m_SensorActivityStatus;
};




class IDialogMessagesInterface
{
public:
	virtual void SendMessageToDialog(SDialogDataMessage *Message) = 0;
	virtual void SendMessageToDialog(SDialogSensorMessage *Message) = 0;
};

