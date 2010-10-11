#pragma once

#include "ISensorEvents.h"

struct SDialogMessage
{
	SDialogMessage(int SensorId, SScannedData ScannedData, CString TimeStamp) 
		: m_SensorId(SensorId), m_ScannedData(ScannedData), m_TimeStamp(TimeStamp) {}

	int				m_SensorId;
	SScannedData	m_ScannedData;
	CString			m_TimeStamp;
};

class IDialogMessagesInterface
{
public:
	virtual void SendMessageToDialog(SDialogMessage *Message) = 0;
};

