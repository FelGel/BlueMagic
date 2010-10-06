
#include "stdafx.h"
#include "BlueMagicBTBMessages.h"
#include "Common/Serializer.h"
#include "Common/DeSerializer.h"

#include "SensorBufferDeSerializer.h"
#include "SensorController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CHECK_PARAM(Prm) \
	Assert(Prm != NULL); \
	if (Prm == NULL) \
	return false;


#define TEXTUAL_BTB_PROTOCOL_MESSAGE_TERMINATOR '\n'
#define TEXTUAL_BTB_PROTOCOL_FIELDS_SEPERATOR	' '
const char Delimeters[2] = {TEXTUAL_BTB_PROTOCOL_FIELDS_SEPERATOR, TEXTUAL_BTB_PROTOCOL_MESSAGE_TERMINATOR};
#define TEXTUAL_BTB_PROTOCOL_MESSAGE_POST_TERMINATOR '\r'


#define DefineAndGetStringParam(param)\
std::string param = MessageStringParser.GetNextToken(Delimeters);										\
if (param.length() <= 0)																				\
{																										\
	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to extract field %s from Textual Data message", #param);	\
	return false;																						\
}



std::string CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(EBlueMagicBTBIncomingMessageType MessageType)
{
	switch(MessageType)
	{
		RETURN_TYPE_STR(BTBKeepAlive);
		RETURN_TYPE_STR(BTBIncomingData);
		RETURN_TYPE_STR(BTBErrorInTopology);
		RETURN_TYPE_STR(BTBInfo);
		RETURN_TYPE_STR(BTBSInfo);
	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown BTB MessageType %d", MessageType);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}


bool CBlueMagicBTBIncomingMessage::IsMessageComplete(BYTE *Data, int DataLength)
{
	return (GetPositionOfNextMessage(Data, DataLength) != -1);
}

int CBlueMagicBTBIncomingMessage::GetPositionOfNextMessage(BYTE *Data, int DataLength)
{
	for (int i = 0; i < DataLength-1; i++)
	{
		if (Data[i] == TEXTUAL_BTB_PROTOCOL_MESSAGE_TERMINATOR && Data[i+1] == TEXTUAL_BTB_PROTOCOL_MESSAGE_POST_TERMINATOR)
			return i + 2;
	}

	return -1;
}

std::string CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString(EBlueMagicBTBOutgoingMessageType MessageType)
{
	switch(MessageType)
	{
		RETURN_TYPE_STR(BTBGetInfo);
		RETURN_TYPE_STR(BTBGetData);
		RETURN_TYPE_STR(BTBDefineTopology);
	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown BTB MessageType %d", MessageType);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}


#if IS_TEXTUAL_BTB_PROTOCOL == 1
CBlueMagicBTBIncomingMessage::CBlueMagicBTBIncomingMessage() : m_MessageLength(0) {}

bool CBlueMagicBTBIncomingMessage::DeSerialize(IDeSerializer* DeSerializer)
{
	m_DeSerializer = DeSerializer;
	bool Result = true;

	bool IsFirstByte = true;
	char Char = TEXTUAL_BTB_PROTOCOL_MESSAGE_TERMINATOR; 
	std::string DataMessageString;
	do
	{
		Result = Result && DeSerializer->GetNextCharField(Char);
		if (IsFirstByte)
			IsFirstByte = false;
		else
			DataMessageString += Char;
	} while (Char != TEXTUAL_BTB_PROTOCOL_MESSAGE_TERMINATOR && Result);

	LogEvent(LE_INFOLOW, __FUNCTION__ ": Received Textual %s message: %s", 
		BlueMagicBTBMessageTypeToString((EBlueMagicBTBIncomingMessageType)MessageType()).c_str(), DataMessageString.c_str());

	// Get PostTermninator in Char:
	Result = Result && DeSerializer->GetNextCharField(Char);

	if (!Result || Char != TEXTUAL_BTB_PROTOCOL_MESSAGE_POST_TERMINATOR)
	{
		LogEvent(LE_INFOLOW, __FUNCTION__ ": %s message is incomplete",
			BlueMagicBTBMessageTypeToString((EBlueMagicBTBIncomingMessageType)MessageType()).c_str());
		return false;
	}

	m_MessageLength = DataMessageString.length() + 2/*first byte + post terminator*/;

	CTokenParser MessageStringParser(DataMessageString.c_str());
	return Parse(MessageStringParser);
}
#endif




CBlueMagicBTBKeepAliveMessage::CBlueMagicBTBKeepAliveMessage(BYTE SensorId, short Clock) 
: m_SensorId(SensorId), m_Clock(Clock) {}

bool CBlueMagicBTBKeepAliveMessage::Serialize(ISerializer* /*Serializer*/) const
{
#if (IS_TEXTUAL_BTB_PROTOCOL == 1)
	return false; // currently BlueMagicLogic never serialize this message
#else
	CHECK_PARAM(Serializer);

	bool Result = true;
	Result = Result && Serializer->AppendByte(m_SensorId);
	Result = Result && Serializer->AppendShort(m_Clock);
	return Result;
#endif
}

#if (IS_TEXTUAL_BTB_PROTOCOL == 1)
bool CBlueMagicBTBKeepAliveMessage::Parse(CTokenParser MessageStringParser)
{
	DefineAndGetStringParam(SensorIDString);
	DefineAndGetStringParam(ClockString);

	m_SensorId = atoi(SensorIDString.c_str());
	m_Clock = atoi(ClockString.c_str());

	LogEvent(LE_INFO, __FUNCTION__ ": KeepAlive message Parsed: SensorId=%d, Clock=%d", 
		m_SensorId, m_Clock);

	return true;
}
#else
bool CBlueMagicBTBKeepAliveMessage::DeSerialize(IDeSerializer* DeSerializer)
{
	CHECK_PARAM(DeSerializer);

	bool Result = true;
	Result = Result && DeSerializer->GetNextByteField(m_SensorId);
	Result = Result && DeSerializer->GetNextShortField(m_Clock);

	return Result;
}
#endif

bool CBlueMagicBTBDataMessage::Serialize(ISerializer* /*Serializer*/) const
{
#if (IS_TEXTUAL_BTB_PROTOCOL == 1)
	return false; // currently BlueMagicLogic never serialize this message
#else
	bool Result = true;

	POSITION pos = m_ScannedDataList.GetHeadPosition();
	for (int i=0; i < m_ScannedDataList.GetCount(); i++)
	{
		SScannedData ScannedData = m_ScannedDataList.GetNext(pos);

		Result = Result && Serializer->AppendByte(ScannedData.SensorId);
		Result = Result && Serializer->AppendShort(ScannedData.Clock);
		Result = Result && Serializer->AppendByte(ScannedData.RSSI);
		Result = Result && Serializer->AppendBuffer(ScannedData.ScannedBDADDRESS, BDADDRESS_LENGTH_IN_BYTES);
	}

	Result = Result && Serializer->AppendByte(0);
	return Result;
#endif
}


#if IS_TEXTUAL_BTB_PROTOCOL == 1
bool CBlueMagicBTBDataMessage::Parse(CTokenParser MessageStringParser)
{
	DefineAndGetStringParam(SensorIDString);
	DefineAndGetStringParam(ClockString);
	DefineAndGetStringParam(RSSIString);
	DefineAndGetStringParam(BDADDRESSString);

	m_SensorId = atoi(SensorIDString.c_str());

	SScannedData ScannedData;
	int Clock = atoi(ClockString.c_str());
	ScannedData.RSSI = atoi(RSSIString.c_str());
	ScannedData.ScannedBDADDRESS = BDADDRESSString;

	ScannedData.Time = ((CSensorBufferDeSerializer *)m_DeSerializer)->GetSensorController()->GetTimeForSensorClock(m_SensorId, Clock);

	LogEvent(LE_INFO, __FUNCTION__ ": Data message Parsed: SensorId=%d, Clock=%d, RSSI=%d, BDADDRESS=%s", 
		m_SensorId, Clock, ScannedData.RSSI, ScannedData.ScannedBDADDRESS.c_str());

	m_ScannedData = ScannedData;

	return true;
}
#else
bool CBlueMagicBTBDataMessage::DeSerialize(IDeSerializer* DeSerializer)
	CHECK_PARAM(DeSerializer);

	UCHAR SensorId = 0;
	bool Result = true;
	do 
	{
		Result = Result && DeSerializer->GetNextByteField(SensorId);

		if (SensorId != 0)
		{
			SScannedData ScannedData;
			ScannedData.SensorId = SensorId;

			Result = Result && DeSerializer->GetNextShortField(ScannedData.Clock);
			Result = Result && DeSerializer->GetNextByteField(ScannedData.RSSI);
			Result = Result && DeSerializer->GetNextBufferField(ScannedData.ScannedBDADDRESS, BDADDRESS_LENGTH_IN_BYTES);

			m_ScannedDataList.AddTail(ScannedData);
		}
	} while (SensorId != 0);

	return Result;
}
int	CBlueMagicBTBDataMessage::MessageLength() const
{
	return m_ScannedDataList.GetCount() * sizeof(SScannedData) + sizeof(BYTE)/*stop-byte*/;
}
#endif

bool CBlueMagicBTBInfoMessage::Serialize(ISerializer* /*Serializer*/) const
{
#if (IS_TEXTUAL_BTB_PROTOCOL == 1)
	return false; // currently BlueMagicLogic never serialize this message
#else
#endif
}

#if (IS_TEXTUAL_BTB_PROTOCOL == 1)
bool CBlueMagicBTBInfoMessage::Parse(CTokenParser MessageStringParser)
{
	DefineAndGetStringParam(SensorIDString);
	DefineAndGetStringParam(ClockString);
	DefineAndGetStringParam(Version);
	DefineAndGetStringParam(SensorBDADDRESS);

	m_SensorInfo.SensorId = atoi(SensorIDString.c_str());
	m_SensorInfo.Clock = atoi(ClockString.c_str());
	m_SensorInfo.Version = atoi(Version.c_str());
	m_SensorInfo.SensorBDADDRESS = SensorBDADDRESS;

	LogEvent(LE_INFO, __FUNCTION__ ": BTB INFO message Parsed: SensorId=%d, Clock=%d, Version=%d, SensorBDADDRESS=%s", 
		m_SensorInfo.SensorId, m_SensorInfo.Clock, m_SensorInfo.Version, m_SensorInfo.SensorBDADDRESS.c_str());

	return true;
}
#else
#endif