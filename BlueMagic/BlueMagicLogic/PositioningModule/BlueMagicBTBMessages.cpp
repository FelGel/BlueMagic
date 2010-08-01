
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


bool CBlueMagicBTBDataMessage::Serialize(ISerializer* Serializer) const
{
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
}

bool CBlueMagicBTBDataMessage::DeSerialize(IDeSerializer* DeSerializer)
{
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