
#include "stdafx.h"
#include "BlueMagicBTBMessages.h"
#include "Common/Serializer.h"
#include "Common/DeSerializer.h"

#define CHECK_PARAM(Prm) \
	Assert(Prm != NULL); \
	if (Prm == NULL) \
	return false;


std::string CBlueMagicBTBMessage::BlueMagicBTBMessageTypeToString(EBlueMagicBTBMessageType MessageType)
{
	switch(MessageType)
	{
		RETURN_TYPE_STR(BTBKeepAlive);
	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown BTB MessageType %d", MessageType);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}


CBlueMagicBTBKeepAliveMessage::CBlueMagicBTBKeepAliveMessage(BYTE SensorId, short Clock) 
: m_SensorId(SensorId), m_Clock(Clock) {}

bool CBlueMagicBTBKeepAliveMessage::Serialize(ISerializer* Serializer) const
{
	CHECK_PARAM(Serializer);

	bool Result = true;
	Result = Result && Serializer->AppendByte(m_SensorId);
	Result = Result && Serializer->AppendShort(m_Clock);
	return Result;
}

bool CBlueMagicBTBKeepAliveMessage::DeSerialize(IDeSerializer* DeSerializer)
{
	CHECK_PARAM(DeSerializer);

	bool Result = true;
	Result = Result && DeSerializer->GetNextByteField(m_SensorId);
	Result = Result && DeSerializer->GetNextShortField(m_Clock);

	return Result;
}
