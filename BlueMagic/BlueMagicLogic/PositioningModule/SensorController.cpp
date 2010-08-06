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
#define TIME_BETWEEN_HANDSHAKE_ATTEMPTS  30000 //milisec


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSensorController::CSensorController(int SensorID, int ComPort, std::string BDADDRESS) 
	: CThreadWithQueue("SensorController", SENSOR_CONTROLLER_QUEUE_SIZE), m_SerialPort(this, ComPort, SensorID),
		m_SensorID(SensorID), m_ComPort(ComPort), m_BDADDRESS(BDADDRESS), m_EventsHandler(NULL), 
		m_ConnectionStatus(SensorNotConnected), m_HandshakeStatus(SensorNotHandshaked),
		m_LastConnectionAttemptTickCount(0), m_LastHandshakeAttemptTickCount(0)
		
{
	InsertValueToMap(m_SensorsDataBuffferMap, SensorID, new SSensorInformation());
}

CSensorController::~CSensorController(void)
{
	Assert(m_SensorsDataBuffferMap.size() == 0);
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

	std::map<int /*SensorId*/, SSensorInformation*>::iterator Iter = m_SensorsDataBuffferMap.begin();
	std::map<int /*SensorId*/, SSensorInformation*>::iterator End = m_SensorsDataBuffferMap.end();

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

	DoHandshake();

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
	LogEvent(LE_DEBUG, __FUNCTION__ ": SensorID=%d DataLength=%d, Data(hexa)=%X, Data(chars)=%s", 
		DataFromSensor.SensorID, DataFromSensor.DataLength, DataFromSensor.Data, DataFromSensor.Data);

	//char hexval[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	//std::string Str;
	//for (int i = 0; i < DataFromSensor.DataLength; i++)
	//{
	//	Str += hexval[((DataFromSensor.Data[i] >> 4) & 0xF)];
	//	Str += hexval[(DataFromSensor.Data[i]) & 0x0F];
	//}

	//LogEvent(LE_INFOHIGH, "CSensorController::HandleDataReceived: SensorID=%d DataLength=%d, Data(hexa)=%X", 
	//	DataFromSensor.SensorID, DataFromSensor.DataLength, DataFromSensor.Data);

	//// TEMP !!!!!!!!!!!!!!!!!!!!!
	//return;


	if (m_ConnectionStatus != SensorConnected)
	{
		Assert(false);
		LogEvent(LE_WARNING, __FUNCTION__ ": Data received on Sensor %d Port %d, while it is assumed to be disconnected !!",
			m_SensorID, m_ComPort);
		// return; - who cares?
	}

	if (m_HandshakeStatus == SensorHandshakeFailed)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Data received on Sensor %d Port %d, yet handshake previously failed !! IGNORING DATA",
			m_SensorID, m_ComPort);
		delete[] DataFromSensor.Data;
		return;
	}

	SSensorInformation* SensorDataBuffer;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, DataFromSensor.SensorID, SensorDataBuffer))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", DataFromSensor.SensorID);
		delete[] DataFromSensor.Data;
		return;
	}

	if (SensorDataBuffer->m_DataBufferOffset + DataFromSensor.DataLength > DATA_BUFFER_SIZE)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": DATA OVERFLOW! MaxSize=%d, DesiredSize=%d"
			,DATA_BUFFER_SIZE, SensorDataBuffer->m_DataBufferOffset + DataFromSensor.DataLength);
		Assert(false);
		// might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
		ResetConnection();
		delete[] DataFromSensor.Data;
		return;
	}

	// Add new data to end of buffer
	memcpy(SensorDataBuffer->m_DataBuffer + SensorDataBuffer->m_DataBufferOffset, DataFromSensor.Data, DataFromSensor.DataLength);
	delete[] DataFromSensor.Data;
	SensorDataBuffer->m_DataBufferOffset += DataFromSensor.DataLength;

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


	if (m_ConnectionStatus != SensorResettingConnection && m_HandshakeStatus != SensorHandshakeFailed)
	{
		// move buffer leftovers to beginning
		if (ParsedBytesSoFar > 0 && ParsedBytesSoFar != SensorDataBuffer->m_DataBufferOffset)
		{
			memcpy(SensorDataBuffer->m_DataBuffer, SensorDataBuffer->m_DataBuffer+ParsedBytesSoFar, 
				SensorDataBuffer->m_DataBufferOffset - ParsedBytesSoFar);
		}

		SensorDataBuffer->m_DataBufferOffset -= ParsedBytesSoFar;
	}
}

DWORD CSensorController::ParseData(int SensorID, BYTE *Data, int DataLength)
{
	// BTB header only consists of MessageType:
	EBlueMagicBTBIncomingMessageType MessageType = (EBlueMagicBTBIncomingMessageType)*(BYTE *)Data;

	if (!IsHeaderValid(MessageType))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Invalid Message: Type [%s]", 
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
		
		// Might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
		ResetConnection(); 
		return 0;
	}

	LogEvent(LE_INFOLOW, __FUNCTION__ ": Message: Type [%s]", 
		CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

	if (m_HandshakeStatus != SensorHandshaked && MessageType != BTBInfo)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": HandShake not performed, refuse processing the message (Type=%s)", 
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

		ResetConnection(); // otherwise they might be out of synch anyway
		//return 0; TEMP
	}

	// Ask the messages factory to create the appropriate message:
	CBlueMagicBTBIncomingMessage* BlueMagicBTBMessage = CreateBlueMagicBTBMessage(MessageType);
	if (BlueMagicBTBMessage == NULL)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": failed to create BlueMagicBTBMessage %s",
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
		return 0;
	}

	// Deserialize it:
	CBufferDeSerializer MessageDeserializer(NULL, (const BYTE *)Data, DataLength, sizeof(BYTE) /*Header=1BYTE(TYPE)*/, true /*IMPORTANT !!*/);
	bool IsCompleteMessage = BlueMagicBTBMessage->DeSerialize(&MessageDeserializer);

	// If message is incomplete return parsed bytes 0.
	if (!IsCompleteMessage)
	{
		delete BlueMagicBTBMessage;
		return 0;
	}
	Assert(BlueMagicBTBMessage->MessageLength() >= 0);
	Assert(BlueMagicBTBMessage->MessageType() == MessageType);

	if (/*m_TraceInterfaceMessages ||*/ GetLogLevel() <= LE_INFOLOW)
	{
		CStringSerializer StringSerializer;
		BlueMagicBTBMessage->Serialize(&StringSerializer);
		LogEvent(LE_INFOLOW, __FUNCTION__ "CSensorController::ParseData: Message: Type [%s] Length [%d] Data [%s]",
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str(), BlueMagicBTBMessage->MessageLength(), StringSerializer.GetString().c_str());
	}

	int MessageSize = BlueMagicBTBMessage->MessageLength();
	int ParsedBytes = 0;
	if (MessageType == BTBInfo) // Handshake Message
	{
		OnBTBInfoMessage((CBlueMagicBTBInfoMessage *)BlueMagicBTBMessage);
		if (m_HandshakeStatus == SensorHandshakeFailed)
			ParsedBytes = 0; // intentionally. so parsing will cease immediately!
		else
			ParsedBytes = MessageSize + sizeof(BYTE);
	}
	else // Any other message
	{
		// Finally, call the appropriate event:
		CallEventOnMessage(SensorID, BlueMagicBTBMessage, DataLength);

		// RETURN size + sizeof(BYTE) for header !!!!
		ParsedBytes = MessageSize + sizeof(BYTE);
	}

	delete BlueMagicBTBMessage;
	return ParsedBytes;
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
	// ToDo - in the future:
	// 1. Allow sending of a message to a specific SensorID (via bridge?)
	// 2. Check handshake of that specific SensorID !!

	if (m_HandshakeStatus == SensorHandshakeFailed)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Attempting to send message %s to sensor %d on port %d who failed handshake. NOT SENDING MESSAGE",
			CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString((EBlueMagicBTBOutgoingMessageType)Message->MessageType()).c_str(), m_SensorID, m_ComPort);

		delete Message;
		return false;
	}

	if (m_HandshakeStatus == SensorNotHandshaked 
		&& Message->MessageType() != BTBGetInfo 
		&& Message->MessageType() != BTBDefineTopology )
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Attempting to send message %s to sensor %d on port %d who has not handshaked. NOT SENDING MESSAGE",
			CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString((EBlueMagicBTBOutgoingMessageType)Message->MessageType()).c_str(), m_SensorID, m_ComPort);

		delete Message;
		return false;
	}

	// Start by Serializing the message, leaving a place for the header
	int HeaderSize = sizeof(BYTE);
	CBufferSerializer Serializer(HeaderSize);
	if (!Message->Serialize(&Serializer))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Error serializing message (Type %d %s)",
			Message->MessageType(), CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString((EBlueMagicBTBOutgoingMessageType)Message->MessageType()).c_str());
		delete Message;
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

	delete Message;
	return IsSent;
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

	if (m_ConnectionStatus == SensorConnected && m_HandshakeStatus == SensorNotHandshaked &&
		TickCount - m_LastHandshakeAttemptTickCount > TIME_BETWEEN_HANDSHAKE_ATTEMPTS)
	{
		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Attempting to send handshake to Sensor..");
		DoHandshake();
	}
}

void CSensorController::StartConnectionRetiresMechanism()
{
	m_ConnectionStatus = SensorAttemptsAtConnection;
	m_LastConnectionAttemptTickCount = GetTickCount();
}

void CSensorController::DoHandshake()
{
	Assert(m_ConnectionStatus == SensorConnected && m_HandshakeStatus != SensorHandshaked);

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Attempting to send handshake (GetInfo) to Sensor..");

	m_LastHandshakeAttemptTickCount = GetTickCount();
	GetInfo();
}

void CSensorController::ResetConnection()
{
	// Clean m_SensorsDataBuffferMap:

}

void CSensorController::OnBTBInfoMessage(CBlueMagicBTBInfoMessage *BTBInfoMessage)
{
	SetClockForSensor(BTBInfoMessage->m_SensorInfo.Clock, BTBInfoMessage->m_SensorInfo.SensorId);

	if (m_HandshakeStatus == SensorHandshakeFailed)
	{
		Assert(false);
		LogEvent(LE_ERROR, __FUNCTION__ ": Should not receive messages on sensor %d port %d with failed handshake",
			m_SensorID, m_ComPort);
		return;
	}

	ESensorHandshakeStatus NewHandshakeStatus = SensorHandshaked;
	if (BTBInfoMessage->m_SensorInfo.Version != BLUEMAGIC_VERSION)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": WRONG VERSION!! Presumably Sensor %d on port %d reports on version %d while software version is %d",
			m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.Version, BLUEMAGIC_VERSION);

		NewHandshakeStatus = SensorHandshakeFailed;
	}

	if (BTBInfoMessage->m_SensorInfo.SensorId != m_SensorID)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": WRONG SensorID!! Presumably Sensor %d on port %d reports SensorID %d",
			m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.SensorId);

		NewHandshakeStatus = SensorHandshakeFailed;
	}

	if (BTBInfoMessage->m_SensorInfo.SensorBDADDRESS != m_BDADDRESS)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": WRONG BDADDRESS!! Presumably Sensor %d on port %d reports BDADDRESS %s while expected %s",
			m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str(), m_BDADDRESS.c_str());

		NewHandshakeStatus = SensorHandshakeFailed;
	}

	Assert(NewHandshakeStatus != SensorNotHandshaked);
	if (m_HandshakeStatus == SensorHandshaked && NewHandshakeStatus == SensorHandshakeFailed)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Sensor %d on port %d succeeded handshake in the past yet failed now !!",
			m_SensorID, m_ComPort);
	}

	if (NewHandshakeStatus == SensorHandshaked)
	{
		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Sensor %d on port %d Handshaked successfully. Version = %d, SensorId = %d, SensorBDADDRESS = %s",
			m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.Version, BTBInfoMessage->m_SensorInfo.SensorId, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str());
	}
	else
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Sensor %d on port %d Handshaked FAILED. Version = %d, SensorId = %d, SensorBDADDRESS = %s",
			m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.Version, BTBInfoMessage->m_SensorInfo.SensorId, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str());
	}

	m_HandshakeStatus = NewHandshakeStatus;
}

void CSensorController::SetClockForSensor(int Clock, int SensorID)
{
	SSensorInformation* SensorInformation;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, SensorID, SensorInformation))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", SensorID);
		return;
	}

	DWORD TickCount = GetTickCount();
	SensorInformation->m_TickCountForClock0 = TickCount - Clock * 1000 /*millisec in sec*/;
}

int CSensorController::GetClockForSensor(int SensorID)
{
	SSensorInformation* SensorInformation;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, SensorID, SensorInformation))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", SensorID);
		return 0;
	}

	return SensorInformation->m_TickCountForClock0;
}


