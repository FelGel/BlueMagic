#pragma once

#include "BlueMagicCommon\BlueMagicMessage.h"
#include "BlueMagicCommon\BlueMagicMessageFactory.h"
#include "Common\Serializer.h"
#include "Common\DeSerializer.h"


#include "ISensorEvents.h"
#include <string>

#define		BLUE_MAGIC_BTB_MESSAGE_MAX_SIZE		1024



// Decoder Messages Enum:
// ----------------------
enum EBlueMagicBTBMessageType
{
	BTBKeepAlive = BlueMagicBlueToothMessageType, 
	BTBIncomingData,
	BTBErrorInTopology,
	BTBInfo,
	BTBSInfo,
	EBlueMagicBTBMessageType_MAX
};
// If more than 19, EIlmMessageType should be changed!!!


// Decoder Message Abstract Parents:
// ---------------------------------
class CBlueMagicBTBMessage : public CBlueMagicMessage
{
public:
	virtual void CallEventOnMessage(ISensorEvents* SensorEvents) const = 0;

	static std::string BlueMagicBTBMessageTypeToString(EBlueMagicBTBMessageType MessageType);
};


// BlueMagicBTBMessageXXXCreator Macro:
// -------------------------------
#define RegisterBlueMagicBTBMessage(Type, MessageClass) RegisterBlueMagicMessage(Type, EBlueMagicBTBMessageType, MessageClass, CBlueMagicBTBMessage)


// Decoder Information Request Message:
// ------------------------------------
class CBlueMagicBTBKeepAliveMessage : public CBlueMagicBTBMessage
{
public:
	CBlueMagicBTBKeepAliveMessage() {}
	CBlueMagicBTBKeepAliveMessage(BYTE SensorId, short Clock);
	virtual ~CBlueMagicBTBKeepAliveMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;
	virtual bool				DeSerialize(IDeSerializer* DeSerializer);

	virtual int					MessageLength() const { return sizeof(m_SensorId)+sizeof(m_Clock); }
	virtual int                 MessageType() const { return BTBKeepAlive; }

	virtual void				CallEventOnMessage(ISensorEvents* /*SensorEvents*/) const
	{ LogEvent(LE_ERROR, "CBlueMagicBTBKeepAliveMessage should not call on event !!"); }

	//Members:
	BYTE m_SensorId;
	short m_Clock;
};
RegisterBlueMagicBTBMessage(BTBKeepAlive, CBlueMagicBTBKeepAliveMessage)


class CBlueMagicBTBDataMessage : public CBlueMagicBTBMessage
{
public:
	CBlueMagicBTBDataMessage() {}
	virtual ~CBlueMagicBTBDataMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;
	virtual bool				DeSerialize(IDeSerializer* DeSerializer);

	virtual int					MessageLength() const;
	virtual int                 MessageType() const { return BTBIncomingData; }

	virtual void				CallEventOnMessage(ISensorEvents* SensorEvents) const
	{ SensorEvents->OnIncomingScannedData((CList<SScannedData> *)&m_ScannedDataList); }

	//Members:
	CList<SScannedData> m_ScannedDataList;
};
RegisterBlueMagicBTBMessage(BTBIncomingData, CBlueMagicBTBDataMessage)
