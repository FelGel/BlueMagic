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
enum EBlueMagicBTBIncomingMessageType
{
	BTBKeepAlive = BlueMagicBTBIncomingMessageType, 
	BTBIncomingData,
	BTBErrorInTopology,
	BTBInfo,
	BTBSInfo,
	EBlueMagicBTBIncomingMessageType_MAX
};
// If more than 10, EBlueMagicMessageType should be changed!!!

enum EBlueMagicBTBOutgoingMessageType
{
	BTBGetInfo = BlueMagicBTBIncomingMessageType, 
	BTBGetData,
	BTBDefineTopology,
	EBlueMagicBTBOutgoingMessageType_MAX
};
// If more than 10, EBlueMagicMessageType should be changed!!!

class CBlueMagicBTBIncomingMessage : public CBlueMagicMessage
{
public:
	virtual void CallEventOnMessage(ISensorEvents* SensorEvents) const = 0;

	static std::string BlueMagicBTBMessageTypeToString(EBlueMagicBTBIncomingMessageType MessageType);
};


class CBlueMagicBTBOutgoingMessage : public CBlueMagicMessage
{
public:
	static std::string BlueMagicBTBMessageTypeToString(EBlueMagicBTBOutgoingMessageType MessageType);
};


// BlueMagicBTBMessageXXXCreator Macro:
// -------------------------------
#define RegisterBlueMagicBTBMessage(Type, MessageClass) RegisterBlueMagicMessage(Type, EBlueMagicBTBIncomingMessageType, MessageClass, CBlueMagicBTBIncomingMessage)


// Decoder Information Request Message:
// ------------------------------------
class CBlueMagicBTBKeepAliveMessage : public CBlueMagicBTBIncomingMessage
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


class CBlueMagicBTBDataMessage : public CBlueMagicBTBIncomingMessage
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


// GetData has ONLY HEADER and EMPTY MESSAGE
class CBlueMagicBTBGetDataMessage : public CBlueMagicBTBOutgoingMessage
{
public:
	CBlueMagicBTBGetDataMessage() {}
	virtual ~CBlueMagicBTBGetDataMessage() {}

	virtual bool				Serialize(ISerializer* /*Serializer*/) const {return true;}
	virtual bool				DeSerialize(IDeSerializer* /*DeSerializer*/) {return true;}

	virtual int					MessageLength() const {return 0;}
	virtual int                 MessageType() const { return BTBGetData; }
};

// GetINFO has ONLY HEADER and EMPTY MESSAGE
class CBlueMagicBTBGetInfoMessage : public CBlueMagicBTBOutgoingMessage
{
public:
	CBlueMagicBTBGetInfoMessage() {}
	virtual ~CBlueMagicBTBGetInfoMessage() {}

	virtual bool				Serialize(ISerializer* /*Serializer*/) const {return true;}
	virtual bool				DeSerialize(IDeSerializer* /*DeSerializer*/) {return true;}

	virtual int					MessageLength() const {return 0;}
	virtual int                 MessageType() const { return BTBGetInfo; }
};