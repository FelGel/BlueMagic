#include "StdAfx.h"

#include <map>

#include "SensorController.h"
#include "Common/collectionhelper.h"
#include "Common/BufferDeSerializer.h"
#include "Common/StringSerializer.h"
#include "BlueMagicBTBMessages.h"
#include "BlueMagicCommon/BlueMagicMessageFactory.h"

#define SENSOR_CONTROLLER_QUEUE_SIZE 10000
#define SENSOR_CONTROLLER_THREAD_TIMEOUT 100 //milisec

CSensorController::CSensorController(int SensorID, int ComPort, std::string BDADDRESS) 
	: CThreadWithQueue("SensorController", SENSOR_CONTROLLER_QUEUE_SIZE), m_SerialPort(this, ComPort, SensorID),
		m_SensorID(SensorID), m_ComPort(ComPort), m_BDADDRESS(BDADDRESS), m_EventsHandler(NULL)
		
{
	InsertValueToMap(m_SensorsDataBuffferMap, SensorID, new SSensorDataBuffer());
}

CSensorController::~CSensorController(void)
{
}

bool CSensorController::Init(ISensorEvents *Handler)
{
	m_EventsHandler = Handler;

	// TODO: connection retries mechanism !!
	ConnectToPort();

	// start thread
	SetTimeout(SENSOR_CONTROLLER_THREAD_TIMEOUT);
	bool Success = StartThread();
	if (!Success)
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not start working thread !!");
		return false;
	}

	return true;
}

void CSensorController::Close()
{
	LogEvent(LE_INFO, "Closing ComPort COM%d of SensorID %d.", 
		m_ComPort, m_SensorID);
	m_SerialPort.Close();

	std::map<int /*SensorId*/, SSensorDataBuffer*>::iterator Iter = m_SensorsDataBuffferMap.begin();
	std::map<int /*SensorId*/, SSensorDataBuffer*>::iterator End = m_SensorsDataBuffferMap.end();

	for(;Iter != End; ++Iter)
	{
		delete Iter->second;
		Iter->second = NULL;
	}

	m_SensorsDataBuffferMap.clear();
}

bool CSensorController::ConnectToPort()
{
	if (!m_SerialPort.ConnectToPort())
	{
		LogEvent(LE_ERROR, "Failed to ConnectToPort COM%d of SensorID %d.", 
			m_ComPort, m_SensorID);
		return false;
	}

	LogEvent(LE_INFOHIGH, "SensorID %d connected successfully to ComPort COM%d", 
		m_SensorID, m_ComPort);
	return true;
}

/*virtual*/ void CSensorController::OnDataReceived(int SerialPortID, BYTE *Data, int DataLength)
{
	SDataFromSensor DataFromSensor;
	DataFromSensor.SensorID = SerialPortID;
	DataFromSensor.Data = Data;
	DataFromSensor.DataLength = DataLength;

	AddHandlerToQueue(&CSensorController::HandleDataReceived, DataFromSensor);
}

void CSensorController::HandleDataReceived(const SDataFromSensor& DataFromSensor)
{
	LogEvent(LE_DEBUG, "CSensorController::HandleDataReceived: SensorID=%d DataLength=%d, Data=%s", 
		DataFromSensor.SensorID, DataFromSensor.DataLength, DataFromSensor.Data);


	SSensorDataBuffer* SensorDataBuffer;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, DataFromSensor.SensorID, SensorDataBuffer))
	{
		LogEvent(LE_ERROR, "CSensorController::HandleDataReceived: SensorID %d is not in map!! cannot handle its data", DataFromSensor.SensorID);
		delete[] DataFromSensor.Data;
		return;
	}

	if (SensorDataBuffer->m_DataBufferOffset + DataFromSensor.DataLength > DATA_BUFFER_SIZE)
	{
		LogEvent(LE_ERROR, "CSensorController::HandleDataReceived: DATA OVERFLOW! MaxSize=%d, DesiredSize=%d"
			,DATA_BUFFER_SIZE, SensorDataBuffer->m_DataBufferOffset + DataFromSensor.DataLength);
		Assert(false);
		// ToDo: might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
	}

	// Add new data to end of buffer
	memcpy(SensorDataBuffer->m_DataBuffer + SensorDataBuffer->m_DataBufferOffset, DataFromSensor.Data, DataFromSensor.DataLength);
	delete[] DataFromSensor.Data;

	// parse all complete messages in buffer
	DWORD ParsedBytes = 0, ParsedBytesSoFar = 0;
	do 
	{
		ParsedBytes = ParseData(DataFromSensor.SensorID, SensorDataBuffer->m_DataBuffer + ParsedBytesSoFar, SensorDataBuffer->m_DataBufferOffset - ParsedBytesSoFar);
		if (ParsedBytes == 0)
			break;
		ParsedBytesSoFar += ParsedBytes;
	}
	while (ParsedBytesSoFar < SensorDataBuffer->m_DataBufferOffset);

	// move buffer leftovers to beginning
	if (ParsedBytesSoFar != SensorDataBuffer->m_DataBufferOffset)
	{
		memcpy(SensorDataBuffer->m_DataBuffer, SensorDataBuffer->m_DataBuffer+ParsedBytesSoFar, 
			SensorDataBuffer->m_DataBufferOffset - ParsedBytesSoFar);
	}

	SensorDataBuffer->m_DataBufferOffset -= ParsedBytesSoFar;
}

DWORD CSensorController::ParseData(int SensorID, BYTE *Data, int DataLength)
{
	// BTB header only consists of MessageType:
	EBlueMagicBTBMessageType MessageType = *(EBlueMagicBTBMessageType *)Data;

	if (!IsHeaderValid(MessageType))
	{
		LogEvent(LE_ERROR, __FUNCTION__ "CSensorController::ParseData: Invalid Message: Type [%s]", 
			CBlueMagicBTBMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
		// ToDo: might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
		return 0;
	}

	LogEvent(LE_INFOLOW, __FUNCTION__ "CSensorController::ParseData: Message: Type [%s]", 
		CBlueMagicBTBMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

	// Ask the messages factory to create the appropriate message:
	CBlueMagicBTBMessage* BlueMagicBTBMessage = CreateBlueMagicBTBMessage(MessageType);
	if (BlueMagicBTBMessage == NULL)
	{
		LogEvent(LE_ERROR, __FUNCTION__ "CSensorController::ParseData: failed to create BlueMagicBTBMessage %s",
			CBlueMagicBTBMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
		return 0;
	}

	// Deserialize it:
	CBufferDeSerializer MessageDeserializer(NULL, (const BYTE *)Data, DataLength, sizeof(BYTE) /*Header=1BYTE(TYPE)*/, true /*IMPORTANT !!*/);
	bool IsCompleteMessage = BlueMagicBTBMessage->DeSerialize(&MessageDeserializer);

	// If message is incomplete return parsed bytes 0.
	if (!IsCompleteMessage)
		return 0;

	if (/*m_TraceInterfaceMessages ||*/ GetLogLevel() <= LE_INFOLOW)
	{
		CStringSerializer StringSerializer;
		BlueMagicBTBMessage->Serialize(&StringSerializer);
		LogEvent(LE_INFOLOW, __FUNCTION__ "CSensorController::ParseData: Message: Type [%s] Length [%d] Data [%s]",
			CBlueMagicBTBMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str(), BlueMagicBTBMessage->MessageLength(), StringSerializer.GetString().c_str());
	}

	// Finally, call the appropriate event:
	Assert(BlueMagicBTBMessage->MessageLength() >= 0);
	CallEventOnMessage(SensorID, BlueMagicBTBMessage, DataLength);

	delete BlueMagicBTBMessage;

	// RETURN size + sizeof(BYTE) for header !!!!
	return BlueMagicBTBMessage->MessageLength() + sizeof(BYTE);
}

bool CSensorController::IsHeaderValid(EBlueMagicBTBMessageType MessageType)
{
	return ((int)MessageType >= BlueMagicBlueToothMessageType) && ((int)MessageType < EBlueMagicBTBMessageType_MAX);
}

CBlueMagicBTBMessage* CSensorController::CreateBlueMagicBTBMessage(EBlueMagicBTBMessageType MessageType)
{
	return CBlueMagicMessageFactory<EBlueMagicBTBMessageType, CBlueMagicBTBMessage>::GetBlueMagicMessageFactory()->CreateBlueMagicMessage(MessageType);
}

void CSensorController::CallEventOnMessage(int /*SensorID*/, const CBlueMagicBTBMessage* Message, UINT /*MessageSize*/)
{
	Assert(m_EventsHandler != NULL);
	if (m_EventsHandler == NULL)
		LogEvent(LE_ERROR, __FUNCTION__ ": m_ServerEvents == NULL");
	else
		Message->CallEventOnMessage(m_EventsHandler);
}