#pragma once

#include "BlueMagicCommon\BlueMagicMessage.h"
#include "BlueMagicCommon\BlueMagicMessageFactory.h"
#include "Common\Serializer.h"
#include "Common\DeSerializer.h"


#include "ISensorEvents.h"
#include <string>

#define		IS_TEXTUAL_BTB_PROTOCOL						1
#define		CAN_IDENTIFY_COMPLETE_MESSAGES_EXTERNALLY	(IS_TEXTUAL_BTB_PROTOCOL == 1) // DO NOT CHANGE THIS LINE! It is derivation of IS_TEXTUAL_BTB_PROTOCOL's value
#define		BLUE_MAGIC_BTB_MESSAGE_MAX_SIZE				1024


// Decoder Messages Enum:
// ----------------------
enum EBlueMagicBTBOutgoingMessageType
{
	BTBGetInfo = BlueMagicBTBOutgoingMessageType, 
	BTBGetData,
	BTBDefineTopology,
	EBlueMagicBTBOutgoingMessageType_MAX
};
// If more than 4, EBlueMagicMessageType should be changed!!!


enum EBlueMagicBTBIncomingMessageType
{
	BTBKeepAlive = BlueMagicBTBIncomingMessageType, 
	BTBIncomingData,
	BTBConnected,
	BTBInfo,
	BTBErrorInTopology,
	BTBSInfo,
	EBlueMagicBTBIncomingMessageType_MAX,

	BTBGetInfoReturned = BTBGetInfo,
};
// If more than 6, EBlueMagicMessageType should be changed!!!



class CBlueMagicBTBIncomingMessage : public CBlueMagicMessage
{
public:
	virtual void CallEventOnMessage(ISensorEvents* SensorEvents) const = 0;

	static std::string BlueMagicBTBMessageTypeToString(EBlueMagicBTBIncomingMessageType MessageType);
	static bool IsMessageComplete(BYTE *Data, int DataLength);
	static int GetPositionOfNextMessage(BYTE *Data, int DataLength);

#if IS_TEXTUAL_BTB_PROTOCOL == 1
	CBlueMagicBTBIncomingMessage();
	virtual bool DeSerialize(IDeSerializer* DeSerializer);
	virtual bool Parse(CTokenParser &MessageStringParser) = 0;
	virtual int MessageLength() const {return m_MessageLength;}
	
	int m_MessageLength;
	IDeSerializer *m_DeSerializer;
#endif

protected:
	virtual bool ParseHeader(CTokenParser &MessageStringParser);

public:
	/*BYTE*/int m_SensorId; /*Originator*/
};


class CBlueMagicBTBOutgoingMessage : public CBlueMagicMessage
{
public:
	static std::string BlueMagicBTBMessageTypeToString(EBlueMagicBTBOutgoingMessageType MessageType);

	static bool SerializeCarriageReturn(ISerializer *Serializer);
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
	virtual ~CBlueMagicBTBKeepAliveMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;
#if IS_TEXTUAL_BTB_PROTOCOL == 1
	virtual bool				Parse(CTokenParser &MessageStringParser);
#else
	virtual bool				DeSerialize(IDeSerializer* DeSerializer);
	virtual int					MessageLength() const { return sizeof(m_SensorId)+sizeof(m_Clock); }
#endif	

	virtual int                 MessageType() const { return BTBKeepAlive; }

	virtual void				CallEventOnMessage(ISensorEvents* /*SensorEvents*/) const
	{ LogEvent(LE_ERROR, "CBlueMagicBTBKeepAliveMessage should not call on event !!"); }

	//Members:
	/*short*/int m_Clock;
};
RegisterBlueMagicBTBMessage(BTBKeepAlive, CBlueMagicBTBKeepAliveMessage)


class CBlueMagicBTBDataMessage : public CBlueMagicBTBIncomingMessage
{
public:
	CBlueMagicBTBDataMessage() {}
	virtual ~CBlueMagicBTBDataMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;

#if IS_TEXTUAL_BTB_PROTOCOL == 1
	virtual bool				Parse(CTokenParser &MessageStringParser);
#else
	virtual bool				DeSerialize(IDeSerializer* DeSerializer);
	virtual int					MessageLength() const;
#endif	

	virtual int                 MessageType() const { return BTBIncomingData; }

	virtual void				CallEventOnMessage(ISensorEvents* SensorEvents) const
	{ SensorEvents->OnIncomingScannedData(m_SensorId, m_ScannedData); }

	//Members:
	SScannedData m_ScannedData;
};
RegisterBlueMagicBTBMessage(BTBIncomingData, CBlueMagicBTBDataMessage)


class CBlueMagicBTBInfoMessage : public CBlueMagicBTBIncomingMessage
{
public:
	CBlueMagicBTBInfoMessage() {}
	virtual ~CBlueMagicBTBInfoMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;

#if IS_TEXTUAL_BTB_PROTOCOL == 1
	virtual bool				Parse(CTokenParser &MessageStringParser);
#else
	virtual bool				DeSerialize(IDeSerializer* DeSerializer);
	virtual int					MessageLength() const;
#endif	

	virtual int                 MessageType() const { return BTBInfo; }

	virtual void				CallEventOnMessage(ISensorEvents* SensorEvents) const
	{ SensorEvents->OnSensorInfo(m_SensorId, m_SensorInfo); }

	//Members:
	SSensorInfo m_SensorInfo;
};
RegisterBlueMagicBTBMessage(BTBInfo, CBlueMagicBTBInfoMessage)



class CBlueMagicBTBGetInfoReturnedMessage : public CBlueMagicBTBIncomingMessage
{
public:
	CBlueMagicBTBGetInfoReturnedMessage() {}
	virtual ~CBlueMagicBTBGetInfoReturnedMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;

	virtual bool				DeSerialize(IDeSerializer* /*DeSerializer*/);
	virtual int					MessageLength() const { return sizeof(BYTE); }

	#if IS_TEXTUAL_BTB_PROTOCOL == 1
		virtual bool				Parse(CTokenParser &MessageStringParser);
	#endif

	virtual int                 MessageType() const { return BTBGetInfoReturned; }

	virtual void				CallEventOnMessage(ISensorEvents* /*SensorEvents*/) const
	{ LogEvent(LE_ERROR, "CBlueMagicBTBGetInfoReturnedMessage should not call on event !!"); }
};
RegisterBlueMagicBTBMessage(BTBGetInfoReturned, CBlueMagicBTBGetInfoReturnedMessage)


class CBlueMagicBTBConnectedMessage : public CBlueMagicBTBIncomingMessage
{
public:
	CBlueMagicBTBConnectedMessage() {}
	virtual ~CBlueMagicBTBConnectedMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const;
#if IS_TEXTUAL_BTB_PROTOCOL == 1
	virtual bool				Parse(CTokenParser &MessageStringParser);
#else
	virtual bool				DeSerialize(IDeSerializer* DeSerializer);
	virtual int					MessageLength() const { return sizeof(m_SensorId)+sizeof(m_Clock); }
#endif	

	virtual int                 MessageType() const { return BTBConnected; }

	virtual void				CallEventOnMessage(ISensorEvents* /*SensorEvents*/) const
	{ LogEvent(LE_ERROR, "CBlueMagicBTBConnectedMessage should not call on event !!"); }

protected:
	virtual bool ParseHeader(CTokenParser &/*MessageStringParser*/) {return true;} /*without parsing !!*/

	//Members:
public:
	std::string m_SensorBDADDRESS;
};
RegisterBlueMagicBTBMessage(BTBConnected, CBlueMagicBTBConnectedMessage)


// GetData has ONLY HEADER and EMPTY MESSAGE
class CBlueMagicBTBGetDataMessage : public CBlueMagicBTBOutgoingMessage
{
public:
	CBlueMagicBTBGetDataMessage() {}
	virtual ~CBlueMagicBTBGetDataMessage() {}

	virtual bool				Serialize(ISerializer* Serializer) const {return CBlueMagicBTBOutgoingMessage::SerializeCarriageReturn(Serializer);}
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

	virtual bool				Serialize(ISerializer* Serializer) const {return CBlueMagicBTBOutgoingMessage::SerializeCarriageReturn(Serializer);}
	virtual bool				DeSerialize(IDeSerializer* /*DeSerializer*/) {return true;}

	virtual int					MessageLength() const {return 0;}
	virtual int                 MessageType() const { return BTBGetInfo; }
};