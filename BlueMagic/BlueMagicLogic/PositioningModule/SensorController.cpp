#include "StdAfx.h"

#include <map>

#include "SensorController.h"
#include "Common/collectionhelper.h"
#include "Common/BufferDeSerializer.h"
#include "Common/BufferSerializer.h"
#include "Common/StringSerializer.h"
#include "BlueMagicBTBMessages.h"
#include "BlueMagicCommon/BlueMagicMessageFactory.h"

#define SENSOR_CONTROLLER_QUEUE_SIZE 10000
#define SENSOR_CONTROLLER_THREAD_TIMEOUT 100 //milisec
#define TIME_BETWEEN_CONNCETION_ATTEMPTS 5000 //milisec


CSensorController::CSensorController(int SensorID, int ComPort, std::string BDADDRESS) 
	: CThreadWithQueue("SensorController", SENSOR_CONTROLLER_QUEUE_SIZE), m_SerialPort(this, ComPort, SensorID),
		m_SensorID(SensorID), m_ComPort(ComPort), m_BDADDRESS(BDADDRESS), m_EventsHandler(NULL), m_ConnectionStatus(SensorNotConnected)
		
{
	InsertValueToMap(m_SensorsDataBuffferMap, SensorID, new SSensorDataBuffer());
}

CSensorController::~CSensorController(void)
{
}

bool CSensorController::Init(ISensorEvents *Handler)
{
	m_EventsHandler = Handler;

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

		StartConnectionRetiresMechanism();

		return false;
	}

	LogEvent(LE_INFOHIGH, "SensorID %d connected successfully to ComPort COM%d", 
		m_SensorID, m_ComPort);

	m_ConnectionStatus = SensorConnected;

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
	EBlueMagicBTBIncomingMessageType MessageType = *(EBlueMagicBTBIncomingMessageType *)Data;

	if (!IsHeaderValid(MessageType))
	{
		LogEvent(LE_ERROR, __FUNCTION__ "CSensorController::ParseData: Invalid Message: Type [%s]", 
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
		// ToDo: might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
		return 0;
	}

	LogEvent(LE_INFOLOW, __FUNCTION__ "CSensorController::ParseData: Message: Type [%s]", 
		CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

	// Ask the messages factory to create the appropriate message:
	CBlueMagicBTBIncomingMessage* BlueMagicBTBMessage = CreateBlueMagicBTBMessage(MessageType);
	if (BlueMagicBTBMessage == NULL)
	{
		LogEvent(LE_ERROR, __FUNCTION__ "CSensorController::ParseData: failed to create BlueMagicBTBMessage %s",
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
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
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str(), BlueMagicBTBMessage->MessageLength(), StringSerializer.GetString().c_str());
	}

	// Finally, call the appropriate event:
	Assert(BlueMagicBTBMessage->MessageLength() >= 0);
	CallEventOnMessage(SensorID, BlueMagicBTBMessage, DataLength);

	delete BlueMagicBTBMessage;

	// RETURN size + sizeof(BYTE) for header !!!!
	return BlueMagicBTBMessage->MessageLength() + sizeof(BYTE);
}

bool CSensorController::IsHeaderValid(EBlueMagicBTBIncomingMessageType MessageType)
{
	return ((int)MessageType >= BlueMagicBTBIncomingMessageType) && ((int)MessageType < EBlueMagicBTBIncomingMessageType_MAX);
}

CBlueMagicBTBIncomingMessage* CSensorController::CreateBlueMagicBTBMessage(EBlueMagicBTBIncomingMessageType MessageType)
{
	return CBlueMagicMessageFactory<EBlueMagicBTBIncomingMessageType, CBlueMagicBTBIncomingMessage>::GetBlueMagicMessageFactory()->CreateBlueMagicMessage(MessageType);
}

void CSensorController::CallEventOnMessage(int /*SensorID*/, const CBlueMagicBTBIncomingMessage* Message, UINT /*MessageSize*/)
{
	Assert(m_EventsHandler != NULL);
	if (m_EventsHandler == NULL)
		LogEvent(LE_ERROR, __FUNCTION__ ": m_ServerEvents == NULL");
	else
		Message->CallEventOnMessage(m_EventsHandler);
}


bool CSensorController::SendBlueMagicMessageToSensor(const CBlueMagicBTBOutgoingMessage* Message, const int& SensorID /*= CTcpSocketServer::SEND_ALL*/)
{
	//CCriticalSectionLocker Lock(m_HandshakeLock);
	//if (CheckHandshake && ConnectionId != CTcpSocketServer::SEND_ALL)
	//{
	//	bool HandShakeOk;
	//	if (!GetValueFromMap(m_HandShakeOkMap, ConnectionId, HandShakeOk))
	//	{
	//		LogEvent(LE_ERROR, __FUNCTION__ ": Connection %d is not open, cannot send the message (App [%s], Interface [%s], Version[%d])",
	//			ConnectionId, m_AppName.c_str(), m_InterfaceName.c_str(), m_Version);
	//		return false;
	//	}
	//	else if (!HandShakeOk)
	//	{
	//		LogEvent(LE_ERROR, __FUNCTION__ ": HandShake not performed for connection %d, refuse sending the message (App [%s], Interface [%s], Version[%d])",
	//			ConnectionId, m_AppName.c_str(), m_InterfaceName.c_str(), m_Version);
	//		return false;
	//	}
	//}

	// Start by Serializing the message, leaving a place for the header
	int HeaderSize = sizeof(BYTE);
	CBufferSerializer Serializer(HeaderSize);
	if (!Message->Serialize(&Serializer))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Error serializing message (Type %d %s)",
			Message->MessageType(), CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString((EBlueMagicBTBOutgoingMessageType)Message->MessageType()).c_str());
		return false;
	}

	// Then, Prepare the header
	EBlueMagicBTBOutgoingMessageType MessageType = (EBlueMagicBTBOutgoingMessageType)Message->MessageType();
	memcpy(Serializer.GetBuffer(), &MessageType, HeaderSize);
	
	if (/*m_TraceInterfaceMessages || */GetLogLevel() <= LE_INFOLOW)
	{
		CStringSerializer StringSerializer;
		Message->Serialize(&StringSerializer);
		LogEvent(LE_INFOLOW, __FUNCTION__ ": Message: Type [%s] Length [%d] Data [%s]",
			CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str(), Serializer.GetSize(), StringSerializer.GetString().c_str());
	}

	bool IsSent = m_SerialPort.SendData(Serializer.GetData(), Serializer.GetSize());
	if (!IsSent)
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to send data on ComPort %d SensorID %d. Data Length = %d",
			m_ComPort, m_SensorID, Serializer.GetSize());

	return IsSent;
	// Last, Send the message itself
	// IMPORTANT: message should be sent in one peace, since socket SendMessage may be called from different threads!
	//bool Ret = true;
	//if (!CheckHandshake || ConnectionId != CTcpSocketServer::SEND_ALL)
	//{
	//	bool SendIlmMessageFromServerToClientSucceeded = m_TcpSocketServer.SendMessage(Serializer.GetData(), Serializer.GetSize(), ConnectionId);
	//	//Assert(SendIlmMessageFromServerToClientSucceeded);
	//	if (!SendIlmMessageFromServerToClientSucceeded)
	//	{
	//		LogEvent(LE_ERROR, __FUNCTION__ "(%s): Error sending message (Type %d) to connection %d",
	//			m_InterfaceName.c_str(), IlmMessage->MessageType(), ConnectionId);
	//		return false;
	//	}
	//}
	//else
	//{
	//	const std::map<int, bool>::const_iterator End = m_HandShakeOkMap.end();
	//	for (std::map<int, bool>::const_iterator Iter = m_HandShakeOkMap.begin(); Iter != End; ++Iter)
	//	{
	//		if (Iter->second) // Handshake is ok!
	//		{
	//			const int& _ConnectionId = Iter->first;
	//			bool SendIlmMessageFromServerToClientSucceeded = m_TcpSocketServer.SendMessage(Serializer.GetData(), Serializer.GetSize(), _ConnectionId);
	//			//Assert(SendIlmMessageFromServerToClientSucceeded);
	//			if (!SendIlmMessageFromServerToClientSucceeded)
	//			{
	//				LogEvent(LE_ERROR, __FUNCTION__ "(%s): Error sending message (Type %d) to connection %d",
	//					m_InterfaceName.c_str(), IlmMessage->MessageType(), _ConnectionId);
	//				Ret = false;
	//			}
	//		}
	//		else
	//			LogEvent(LE_INFO, __FUNCTION__ "(%s) SEND_ALL: connection %d without handshake - message will not be sent",
	//			m_InterfaceName.c_str(), Iter->first);
	//	}
	//}

	//return Ret;
}

void CSensorController::GetInfo()
{
	
	AddHandlerToQueue(&CSensorController::HandleGetInfo);
}

void CSensorController::GetData()
{
	
	AddHandlerToQueue(&CSensorController::HandleGetData);
}

void CSensorController::DefineTopology(/*......*/)
{
	AddHandlerToQueue(&CSensorController::HandleDefineTopology);
}

void CSensorController::HandleGetInfo()
{
	CBlueMagicBTBOutgoingMessage *Message = new CBlueMagicBTBGetInfoMessage();
	SendBlueMagicMessageToSensor(Message, m_SensorID);
}

void CSensorController::HandleGetData()
{
	CBlueMagicBTBOutgoingMessage *Message = new CBlueMagicBTBGetDataMessage();
	SendBlueMagicMessageToSensor(Message, m_SensorID);
}

void CSensorController::HandleDefineTopology(/*......*/)
{
}

void CSensorController::OnTimeout()
{
	// Connection Retries Mechanism:
	DWORD TickCount = GetTickCount();
	if (m_ConnectionStatus == SensorAttemptsAtConnection &&
		TickCount - m_LastConnectionAttemptTickCount > TIME_BETWEEN_CONNCETION_ATTEMPTS)
	{
		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Attempting to reconnect to Sensor..");
		ConnectToPort();
	}
}

void CSensorController::StartConnectionRetiresMechanism()
{
	m_ConnectionStatus = SensorAttemptsAtConnection;
	m_LastConnectionAttemptTickCount = GetTickCount();
}