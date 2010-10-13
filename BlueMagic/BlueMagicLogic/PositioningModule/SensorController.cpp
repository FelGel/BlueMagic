#include "StdAfx.h"

#include <map>

#include "SensorController.h"
#include "Common/Config.h"
#include "Common/Utils.h"
#include "Common/collectionhelper.h"
#include "Common/BufferDeSerializer.h"
#include "SensorBufferDeSerializer.h"
#include "Common/BufferSerializer.h"
#include "Common/StringSerializer.h"
#include "BlueMagicBTBMessages.h"
#include "BlueMagicCommon/BlueMagicMessageFactory.h"

#define SENSOR_CONTROLLER_QUEUE_SIZE		10000
#define SENSOR_CONTROLLER_THREAD_TIMEOUT	100 //milisec
#define DEFAULT_TIME_BETWEEN_CONNCETION_ATTEMPTS	10000 //milisec
#define DEFAULT_TIME_BETWEEN_HANDSHAKE_ATTEMPTS		5000 //milisec
#define DEFAULT_TIME_BETWEEN_KEEP_ALIVES			60000 //milisec
#define DEFAULT_STATUS_UPDATE_RESOLUTION			60000 //milisec
#define KEEP_ALIVE_RESOLUTION						5000  //milisec

#define TICKS_IN_SECOND(x) ((x) * 1000)

static const char* GENERAL_SENSORS_CONFIG_SECTION = "GeneralSensorsConfiguration";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSensorController::CSensorController(int SensorID, int ComPort, std::string BDADDRESS, std::vector<int> ChildrenSensorIDs) 
	: CThreadWithQueue("SensorController", SENSOR_CONTROLLER_QUEUE_SIZE), m_SerialPort(this, ComPort, SensorID),
		m_SensorID(SensorID), m_ComPort(ComPort), m_BDADDRESS(BDADDRESS), m_EventsHandler(NULL), m_DataBufferOffset(0),
		m_PhysicalConnectionStatus(SensorNotConnected), m_LastConnectionAttemptTickCount(0), m_LastHandshakeAttemptTickCount(0), m_LastKeepAliveTestingTickCount(0),
		m_StatusUpdateResolution(DEFAULT_STATUS_UPDATE_RESOLUTION), m_LastStatusUpdateTickCount(0),
		m_TimeBetweenConnectionAttempts(DEFAULT_TIME_BETWEEN_CONNCETION_ATTEMPTS), m_TimeBetweenHandshakeAttempts(DEFAULT_TIME_BETWEEN_HANDSHAKE_ATTEMPTS), 
		m_AllowedTimeBetweenKeepAlives(DEFAULT_TIME_BETWEEN_KEEP_ALIVES), m_DebugFlag1(true)
{
	InsertValueToMap(m_SensorsDataBuffferMap, SensorID, new SSensorInformation(BDADDRESS, SensorID, ChildrenSensorIDs));
}

CSensorController::~CSensorController(void)
{
	Assert(m_SensorsDataBuffferMap.size() == 0);
}

bool CSensorController::Init(ISensorEvents *Handler)
{
	m_EventsHandler = Handler;

	ReadGeneralSensorsConfiguration();

	if (!BuildSensorControllerInfoTree())
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not build Info Tree for Sensor Controller %d!!", m_SensorID);
		return false;
	}

	ReportSensorsStatusToPositionManager();

	ConnectToPort();

	// start thread
	SetTimeout(SENSOR_CONTROLLER_THREAD_TIMEOUT);
	bool Success = StartThread();
	if (!Success)
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": FATAL ERROR! Could not start working thread !!");
		return false;
	}

	DoHandshake();

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

	m_PhysicalConnectionStatus = SensorConnected;
	SendStatusUpdate(GetSensorControllerInfo());

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

	Assert(DataFromSensor.SensorID == m_SensorID);
	if (DataFromSensor.SensorID != m_SensorID)
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorController %d has received a message from other SensorController, %d",
			m_SensorID, DataFromSensor.SensorID);

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


	if (m_PhysicalConnectionStatus != SensorConnected)
	{
		Assert(false);
		LogEvent(LE_WARNING, __FUNCTION__ ": Data received on Sensor %d Port %d, while it is assumed to be disconnected !!",
			m_SensorID, m_ComPort);
		// return; - who cares?
	}

	if (GetSensorControllerInfo()->m_HandshakeStatus == SensorHandshakeFailed)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Data received on Sensor Controller %d Port %d, yet handshake of Sensor Controller previously failed !! IGNORING DATA",
			m_SensorID, m_ComPort);
		delete[] DataFromSensor.Data;
		return;
	}

	if (m_DataBufferOffset + DataFromSensor.DataLength > DATA_BUFFER_SIZE)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": DATA OVERFLOW! MaxSize=%d, DesiredSize=%d"
			,DATA_BUFFER_SIZE, m_DataBufferOffset + DataFromSensor.DataLength);
		Assert(false);
		// might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
		ResetConnection(m_SensorID);
		delete[] DataFromSensor.Data;
		return;
	}

	// Add new data to end of buffer
	memcpy(m_DataBuffer + m_DataBufferOffset, DataFromSensor.Data, DataFromSensor.DataLength);
	delete[] DataFromSensor.Data;
	m_DataBufferOffset += DataFromSensor.DataLength;

	// parse all complete messages in buffer
	DWORD ParsedBytes = 0, ParsedBytesSoFar = 0;
	do 
	{
		BYTE *CurrentPositionInBuffer = m_DataBuffer + ParsedBytesSoFar;
		int CurrentBufferSize = m_DataBufferOffset - ParsedBytesSoFar;

		if (CAN_IDENTIFY_COMPLETE_MESSAGES_EXTERNALLY && !IsMessageComplete(CurrentPositionInBuffer, CurrentBufferSize))
		{
			LogEvent(LE_INFOLOW, __FUNCTION__ ": There is not yet a complete message on Sensor %d Port %d.",
				m_SensorID, m_ComPort);
			break; // there's no complete message in buffer
		}

		ParsedBytes = ParseData(CurrentPositionInBuffer, CurrentBufferSize);
		if (ParsedBytes == 0)
			break;
		ParsedBytesSoFar += ParsedBytes;
	}
	while (ParsedBytesSoFar < m_DataBufferOffset);


	if (m_PhysicalConnectionStatus != SensorResettingConnection 
		&& GetSensorControllerInfo()->m_HandshakeStatus != SensorHandshakeFailed)
	{
		// move buffer leftovers to beginning
		if (ParsedBytesSoFar > 0 && ParsedBytesSoFar != m_DataBufferOffset)
		{
			memcpy(m_DataBuffer, m_DataBuffer+ParsedBytesSoFar, 
				m_DataBufferOffset - ParsedBytesSoFar);
		}

		m_DataBufferOffset -= ParsedBytesSoFar;
	}
	else
		LogEvent(LE_WARNING, __FUNCTION__ ": either handshake failed or PhysicalConnectionStatus is resetting for sensor %d", m_SensorID);
}

DWORD CSensorController::ParseData(BYTE *Data, int DataLength)
{
	// First examine MessageType:
	EBlueMagicBTBIncomingMessageType MessageType = (EBlueMagicBTBIncomingMessageType)*(BYTE *)Data;

	if (!IsHeaderValid(MessageType))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Invalid Message: Type [%s]", 
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());
		
		return ParseInvalidData(Data, DataLength);
	}

	LogEvent(LE_INFOLOW, __FUNCTION__ ": Message: Type [%s]", 
		CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

	// Verify SensorController is properly Handshaked
	if (GetSensorControllerInfo()->m_HandshakeStatus != SensorHandshaked && (MessageType != BTBInfo))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Sensor Controller %d HandShake not performed, refuse processing the message (Type=%s)", 
			m_SensorID, CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

		return ParseInvalidData(Data, DataLength);
	}

	// Ask the messages factory to create the appropriate message:
	CBlueMagicBTBIncomingMessage* BlueMagicBTBMessage = CreateBlueMagicBTBMessage(MessageType);
	if (BlueMagicBTBMessage == NULL)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": failed to create BlueMagicBTBMessage %s",
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

		return ParseInvalidData(Data, DataLength);
	}

	// Deserialize it:
	CSensorBufferDeSerializer MessageDeserializer(this, NULL, (const BYTE *)Data, DataLength, sizeof(BYTE) /*Header=1BYTE(TYPE)*/, true /*IMPORTANT !!*/);
	bool IsCompleteMessage = BlueMagicBTBMessage->DeSerialize(&MessageDeserializer);

	// If message is incomplete return parsed bytes 0.
	if (!IsCompleteMessage)
	{
		Assert(!CAN_IDENTIFY_COMPLETE_MESSAGES_EXTERNALLY);
		
		delete BlueMagicBTBMessage;
		return 0;
	}
	Assert(BlueMagicBTBMessage->MessageLength() >= 0);
	Assert(BlueMagicBTBMessage->MessageType() == MessageType);

	
	// RETURN ParsedBytes = size + sizeof(BYTE) for header !!!!
	int MessageSize = BlueMagicBTBMessage->MessageLength();
	int ParsedBytes = MessageSize + sizeof(BYTE);


	// Extract SSensorInformation based on Sending Sensor in Header
	SSensorInformation* SensorInfo;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, BlueMagicBTBMessage->m_SensorId, SensorInfo))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", BlueMagicBTBMessage->m_SensorId);
		
		delete BlueMagicBTBMessage;
		return ParsedBytes; // After all, PARSING in itself has SUCCEEDED !
	}

	// Verify The Sensor Controller is properly Handshaked
	if (GetSensorControllerInfo()->m_HandshakeStatus != SensorHandshaked 
		&& (MessageType != BTBInfo || GetSensorControllerInfo()->m_SensorID != BlueMagicBTBMessage->m_SensorId))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Sensor Controller %d HandShake not performed, refuse processing the message (Type=%s) from SensorID %d", 
			m_SensorID, CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str(), BlueMagicBTBMessage->m_SensorId);

		delete BlueMagicBTBMessage;
		return ParsedBytes; // After all, PARSING in itself has SUCCEEDED !
	}

	// Verify The Sending Sensor is properly Handshaked
	if (SensorInfo->m_HandshakeStatus != SensorHandshaked && MessageType != BTBInfo)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": HandShake not performed on Sensor %d (via Sensor Controller %d), refuse processing the message (Type=%s)", 
			SensorInfo->m_SensorID, m_SensorID, CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str());

		DoHandshake();

		delete BlueMagicBTBMessage;
		return ParsedBytes; // After all, PARSING in itself has SUCCEEDED !
	}

	OnValidMessageArrived(SensorInfo->m_SensorID);

	if (/*m_TraceInterfaceMessages ||*/ GetLogLevel() <= LE_INFOLOW)
	{
		CStringSerializer StringSerializer;
		BlueMagicBTBMessage->Serialize(&StringSerializer);
		LogEvent(LE_INFOLOW, __FUNCTION__ "CSensorController::ParseData: Message: Type [%s] Length [%d] Data [%s]",
			CBlueMagicBTBIncomingMessage::BlueMagicBTBMessageTypeToString(MessageType).c_str(), BlueMagicBTBMessage->MessageLength(), StringSerializer.GetString().c_str());
	}

	if (MessageType == BTBInfo) // Handshake Message
	{
		OnBTBInfoMessage((CBlueMagicBTBInfoMessage *)BlueMagicBTBMessage, SensorInfo);
		
		if (GetSensorControllerInfo()->m_HandshakeStatus == SensorHandshakeFailed)
			ParsedBytes = 0; // intentionally. so parsing will cease immediately!
	}
	else if (MessageType == BTBKeepAlive)
	{
		OnBTBKeepAliveMessage((CBlueMagicBTBKeepAliveMessage *)BlueMagicBTBMessage);
	}
	else // Any other message
	{
		// Finally, call the appropriate event:
		CallEventOnMessage(BlueMagicBTBMessage, DataLength);
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

void CSensorController::CallEventOnMessage(const CBlueMagicBTBIncomingMessage* Message, UINT /*MessageSize*/)
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

	if (GetSensorControllerInfo()->m_HandshakeStatus == SensorHandshakeFailed)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Attempting to send message %s to sensor %d on port %d who failed handshake. NOT SENDING MESSAGE",
			CBlueMagicBTBOutgoingMessage::BlueMagicBTBMessageTypeToString((EBlueMagicBTBOutgoingMessageType)Message->MessageType()).c_str(), m_SensorID, m_ComPort);

		delete Message;
		return false;
	}

	if (GetSensorControllerInfo()->m_HandshakeStatus == SensorNotHandshaked 
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
	// ToDo
}

void CSensorController::OnTimeout()
{
	DWORD TickCount = GetTickCount();
	
	CheckPhysicalConnectionStatus(TickCount);

	CheckHandshakeStatus(TickCount);

	CheckLogicalConnectionStatus(TickCount);

	DoStatusUpdateIfNecessary(TickCount);
}

void CSensorController::StartConnectionRetiresMechanism()
{
	m_PhysicalConnectionStatus = SensorAttemptsAtConnection;
	m_LastConnectionAttemptTickCount = GetTickCount();
	SendStatusUpdate(GetSensorControllerInfo());
}

void CSensorController::DoHandshake()
{
	Assert(m_PhysicalConnectionStatus == SensorConnected 
		&& GetSensorControllerBranchHandshakeStatus() != SensorHandshaked);

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": Attempting to send handshake (GetInfo) to Sensor Controller %d..", m_SensorID);

	m_LastHandshakeAttemptTickCount = GetTickCount();
	GetInfo();
}

void CSensorController::ResetConnection(int SensorID)
{
	LogEvent(LE_WARNING, __FUNCTION__ "Resetting Sensor %d", SensorID);

	// ToDo - Implement this !!
	//Assert(false); // TEMPORARY.
	if (m_DebugFlag1)
	{
		m_DebugFlag1 = false;
		LogEvent(LE_ERROR, __FUNCTION__ "RESET NOT IMPLEMENTED YET !!");
	}

	// Clean m_SensorsDataBuffferMap:
}

void CSensorController::OnBTBInfoMessage(CBlueMagicBTBInfoMessage *BTBInfoMessage, SSensorInformation* SensorInfo)
{
	SetClockForSensor(BTBInfoMessage->m_SensorInfo.Clock, BTBInfoMessage->m_SensorId);

	if (SensorInfo->m_HandshakeStatus == SensorHandshakeFailed)
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

	if (BTBInfoMessage->m_SensorId != SensorInfo->m_SensorID)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": WRONG SensorID!! Presumably Sensor %d (via port %d) reports SensorID %d",
			SensorInfo->m_SensorID, m_ComPort, BTBInfoMessage->m_SensorId);

		NewHandshakeStatus = SensorHandshakeFailed;
	}

	if (BTBInfoMessage->m_SensorInfo.SensorBDADDRESS != SensorInfo->m_BDADDRESS)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": WRONG BDADDRESS!! Presumably Sensor %d (via port %d) reports BDADDRESS %s while expected %s",
			SensorInfo->m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str(), m_BDADDRESS.c_str());

		NewHandshakeStatus = SensorHandshakeFailed;
	}

	Assert(NewHandshakeStatus != SensorNotHandshaked);
	if (SensorInfo->m_HandshakeStatus == SensorHandshaked && NewHandshakeStatus == SensorHandshakeFailed)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Sensor %d (via port %d) succeeded handshake in the past yet failed now !!",
			SensorInfo->m_SensorID, m_ComPort);
	}

	if (NewHandshakeStatus == SensorHandshaked)
	{
		if (SensorInfo->m_HandshakeStatus != SensorHandshaked)
			LogEvent(LE_INFOHIGH, __FUNCTION__ ": Sensor %d on (via port %d) Handshaked successfully. Version = %d, SensorId = %d, SensorBDADDRESS = %s",
				SensorInfo->m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.Version, BTBInfoMessage->m_SensorId, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str());
		else
			LogEvent(LE_WARNING, __FUNCTION__ ": Sensor %d on (via port %d) Handshake verified. Version = %d, SensorId = %d, SensorBDADDRESS = %s",
			SensorInfo->m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.Version, BTBInfoMessage->m_SensorId, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str());
	}
	else
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Sensor %d on (via port %d) Handshaked FAILED. Version = %d, SensorId = %d, SensorBDADDRESS = %s",
			SensorInfo->m_SensorID, m_ComPort, BTBInfoMessage->m_SensorInfo.Version, BTBInfoMessage->m_SensorId, BTBInfoMessage->m_SensorInfo.SensorBDADDRESS.c_str());
	}

	SensorInfo->m_HandshakeStatus = NewHandshakeStatus;
	SendStatusUpdate(SensorInfo);
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
	SensorInformation->m_ClockCorrelationData.MatchingTickCount = TickCount;
	SensorInformation->m_ClockCorrelationData.SensorClock = Clock;

	//SensorInformation->m_TickCountForClock0 = TickCount - Clock * 1000 /*millisec in sec*/;

	// NOTE: Clock might be negative
}

DWORD CSensorController::GetTimeForSensorClock(int SensorID, int Clock)
{
	SSensorInformation* SensorInformation;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, SensorID, SensorInformation))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", SensorID);
		return 0;
	}

	if (SensorInformation->m_ClockCorrelationData.SensorClock > MAX_SENSOR_CLOCK_VALUE)
	{
		LogEvent(LE_WARNING, __FUNCTION__ ": Clock information has not been received yet for Sensor %d !!", SensorID);
		return 0;
	}

	// Assumptions:
	// 1. SensorClock was update BEFORE current Clock!
	// 2. A full int16 loop HAS NOT ELAPSED since last Clock Update!

	DWORD Time = 0;
	if (Clock >= SensorInformation->m_ClockCorrelationData.SensorClock)
		Time = SensorInformation->m_ClockCorrelationData.MatchingTickCount 
			+ TICKS_IN_SECOND(Clock - SensorInformation->m_ClockCorrelationData.SensorClock);
	else
		Time = SensorInformation->m_ClockCorrelationData.MatchingTickCount 
		+ TICKS_IN_SECOND(MAX_SENSOR_CLOCK_VALUE - SensorInformation->m_ClockCorrelationData.SensorClock)
		+ TICKS_IN_SECOND(Clock - MIN_SENSOR_CLOCK_VALUE );

	LogEvent(LE_INFOLOW, __FUNCTION__ ": SensorID = %d, Clock =%d, LastSyncClock = %d, LastTickCount = %d, CalculatedTime = %d",
		SensorID, Clock, SensorInformation->m_ClockCorrelationData.SensorClock, 
		SensorInformation->m_ClockCorrelationData.MatchingTickCount, Time);

	return Time;
}

bool CSensorController::IsMessageComplete(BYTE *Data, int DataLength)
{
	Assert(CAN_IDENTIFY_COMPLETE_MESSAGES_EXTERNALLY);
	if (!CAN_IDENTIFY_COMPLETE_MESSAGES_EXTERNALLY)
		return true;

	return CBlueMagicBTBIncomingMessage::IsMessageComplete(Data, DataLength);
}

DWORD CSensorController::ParseInvalidData(BYTE *Data, int DataLength)
{
	if (CAN_IDENTIFY_COMPLETE_MESSAGES_EXTERNALLY) // then we can simply skip to next message
	{
		if (IsMessageComplete(Data, DataLength))
		{
			LogEvent(LE_WARNING, "Parsing Invalid data on SensorID %d. Skipping to next message", m_SensorID);
			return CBlueMagicBTBIncomingMessage::GetPositionOfNextMessage(Data, DataLength);
		}
		else // probably will be possible next packet, with more data
		{
			LogEvent(LE_WARNING, "Parsing Invalid data on SensorID %d. Waiting for more data", m_SensorID);
			return 0;
		}
	}
	else
	{
		LogEvent(LE_ERROR, "Parsing Invalid data on SensorID %d. Resetting Connection!", m_SensorID);
		// Might get out of sync!! cannot just delete.
		// Best easiest approach: Disconnect and Reconnect to BTB.
		ResetConnection(m_SensorID); 
		return 0;

	}
}

void CSensorController::OnConnectionTimedOut(SSensorInformation *SensorInformation)
{
	if (SensorInformation->m_LogicalConnectionStatus == SensorActive)
	{
		LogEvent(LE_WARNING, __FUNCTION__ ": Connection to Sensor %d (via Sensor Controller %d & COM %d) TimedOut !",
			SensorInformation->m_SensorID, m_SensorID, m_ComPort);

		SensorInformation->m_LogicalConnectionStatus = SensorNotActive;	

		SendStatusUpdate(SensorInformation);
	}
	else
	{
		// will be handled by periodic status dumps
		//LogEvent(LE_WARNING, __FUNCTION__ ": Connection to Sensor %d (via Sensor Controller %d & COM %d) is still INACTIVE !",
		//	SensorInformation->m_SensorID, m_SensorID, m_ComPort);
	}

	ResetConnection(SensorInformation->m_SensorID);
}


void CSensorController::OnBTBKeepAliveMessage(CBlueMagicBTBKeepAliveMessage *BTBKeepAliveMessage)
{
	SetClockForSensor(BTBKeepAliveMessage->m_Clock, BTBKeepAliveMessage->m_SensorId);
}

void CSensorController::OnValidMessageArrived(int SensorID)
{	
	SSensorInformation* SensorInformation;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, SensorID, SensorInformation))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", SensorID);
		return;
	}

	if (SensorInformation->m_LogicalConnectionStatus == SensorNotActive)
		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Sensor %d has just became ACTIVE again", SensorID);

	// Update Originating Sensor
	SensorIsActive(SensorInformation);

	// Update Sensor Controller
	SensorIsActive(GetSensorControllerInfo());

	// NOTE: in theory, we could have updated all connecting links.
	// However, since we do have KeepAlives - no need.

	// As for The Sensor Controller - since it is a physical entity, perhaps
	// it's best to keep it updated.
}

void CSensorController::SensorIsActive(SSensorInformation *SensorInformation)
{
	DWORD TickCount = GetTickCount();

	SensorInformation->m_LastPacketRecievedTickCount = TickCount;
	SensorInformation->m_LogicalConnectionStatus = SensorActive;
	SendStatusUpdate(SensorInformation);
}

SSensorInformation* CSensorController::GetSensorControllerInfo()
{
	SSensorInformation *SensorControllerInfo = NULL;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, m_SensorID, SensorControllerInfo))
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": SensorID %d (SENSOR CONTROLLER) is not in map!! FATAL ERROR", m_SensorID);
		return NULL;
	}

	return SensorControllerInfo;
}


void CSensorController::CheckHandshakeStatus(DWORD TickCount)
{
	if (m_PhysicalConnectionStatus == SensorConnected 
		&& TickCount - m_LastHandshakeAttemptTickCount > m_TimeBetweenHandshakeAttempts
		&& GetSensorControllerBranchHandshakeStatus() == SensorNotHandshaked)
	{
		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Attempting to send handshake to Sensor..");
		DoHandshake();
	}
}

void CSensorController::CheckPhysicalConnectionStatus(DWORD TickCount)
{
	// Connection Retries Mechanism:

	if (m_PhysicalConnectionStatus == SensorAttemptsAtConnection &&
		TickCount - m_LastConnectionAttemptTickCount > m_TimeBetweenConnectionAttempts)
	{
		LogEvent(LE_INFOHIGH, __FUNCTION__ ": Attempting to reconnect to Sensor..");
		ConnectToPort();
	}
}

void CSensorController::CheckLogicalConnectionStatus(DWORD TickCount)
{

	if (TickCount - m_LastKeepAliveTestingTickCount < KEEP_ALIVE_RESOLUTION)
		return;

	m_LastKeepAliveTestingTickCount = TickCount;

	std::map<int /*SensorId*/, SSensorInformation*>::iterator Iter = m_SensorsDataBuffferMap.begin();
	std::map<int /*SensorId*/, SSensorInformation*>::iterator End = m_SensorsDataBuffferMap.end();

	for(;Iter != End; ++Iter)
	{
		SSensorInformation *SensorInformation = Iter->second;
		
		if (m_PhysicalConnectionStatus == SensorConnected && SensorInformation->m_HandshakeStatus == SensorHandshaked &&
			TickCount - SensorInformation->m_LastPacketRecievedTickCount > m_AllowedTimeBetweenKeepAlives)
		{
			OnConnectionTimedOut(SensorInformation);
		}
	}
}

ESensorHandshakeStatus CSensorController::GetSensorBranchHandshakeStatus(int SensorID)
{
	/*
		If at least one Sensor in Branch is Not Handshaked - return value will be SensorNotHandshaked.
		Otherwise, if one sensor has failed its handshake - return value will be SensorHandshakeFailed.
		Otherwise - return value will be SensorHandshaked.
	*/
	SSensorInformation* SensorInformation;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, SensorID, SensorInformation))
	{
		Assert(false);
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", SensorID);
		return SensorHandshakeFailed;
	}

	bool HaveHandshakedFailed = false;
	for (unsigned int i = 0; i < SensorInformation->m_ChildrenSensorIDs.size() ; i++)
	{
		ESensorHandshakeStatus SensorHandshakeStatus = GetSensorBranchHandshakeStatus(SensorInformation->m_ChildrenSensorIDs[i]);
		if (SensorHandshakeStatus == SensorNotHandshaked)
			return SensorHandshakeStatus;

		if (SensorHandshakeStatus == SensorHandshakeFailed)
			HaveHandshakedFailed = true;
	}

	// if we got here, than all other sensors have either Failed or Succeeded
	if (HaveHandshakedFailed)
		return SensorHandshakeFailed;

	// if we got here, than all other sensors have Succeeded
	return SensorInformation->m_HandshakeStatus;
}

ESensorHandshakeStatus CSensorController::GetSensorControllerBranchHandshakeStatus()
{
	return GetSensorBranchHandshakeStatus(m_SensorID);
}

bool CSensorController::BuildSensorControllerInfoTree()
{
	return BuildSensorControllerInfoBranch(m_SensorID);
}

bool CSensorController::BuildSensorControllerInfoBranch(int SensorID)
{
	bool IsOk = true;

	SSensorInformation* SensorInformation;
	if (!GetValueFromMap(m_SensorsDataBuffferMap, SensorID, SensorInformation))
	{
		Assert(false);
		LogEvent(LE_ERROR, __FUNCTION__ ": SensorID %d is not in map!! cannot handle its data", SensorID);
		return false;
	}

	for (unsigned int i = 0; i < SensorInformation->m_ChildrenSensorIDs.size(); i++)	
		IsOk = IsOk && LookupRemoteSensorInConfigurationAndParse(SensorInformation->m_ChildrenSensorIDs[i]);

	return IsOk;
}

bool CSensorController::LookupRemoteSensorInConfigurationAndParse(int SensorID)
{
	std::vector<ConfigListItem> ObjectSections;
	GetListSection("RemoteSensors", "RemoteSensor", ObjectSections);
	//////////////////////////////////////////////////////////////////////////

	if (ObjectSections.size() == 0 && GetSensorControllerInfo()->m_ChildrenSensorIDs.size() > 0)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to read RemoteSensors objects from Configuration !");
		return false;
	}

	for (int i = 0; (unsigned)i < ObjectSections.size(); ++i) 
	{
		std::string ObjectSection = ObjectSections[i].ItemValue;
		
		int ObjectSensorID = GetIntValue(ObjectSection.c_str(), "SensorID=", -1);//GetConfigSectionId(ConfigSection);   
		if (SensorID == ObjectSensorID)
			return ParseRemoteSensorsConfiguration(ObjectSections[i].ItemId, ObjectSection);
	}

	LogEvent(LE_ERROR, __FUNCTION__ ": Failed to locate RemoteSensor %d in Configuration!", SensorID);
	return false;
}

bool CSensorController::ParseRemoteSensorsConfiguration(int ObjectIndex, std::string ConfigSection)
{
	int SensorID = GetIntValue(ConfigSection.c_str(), "SensorID=", -1);//GetConfigSectionId(ConfigSection);   
	std::string BDADDRESS = GetStringValue(ConfigSection.c_str(), "BDADDRESS=", "");

	std::string ChildrenStr = GetStringValue(ConfigSection.c_str(), "Children=", "");

	if (ChildrenStr == "")
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Empty Children list (Index %d SensorID %d)! If this is intentional, enter the keyword 'None' instead!", 
			ObjectIndex, SensorID);
		return false;
	}

	std::vector<int> ChildrenSensorIDs;
	if (ChildrenStr != "None")
	{
		ParseIntVectorString(ChildrenStr.c_str(), ChildrenSensorIDs);
		if (ChildrenSensorIDs.size() == 0)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Empty or Invalid Children list (Index %d SensorID %d)! If this is intentional, enter the keyword 'None' instead!", 
				ObjectIndex, SensorID);
			return false;
		}
	}

	if (!InsertValueToMap(m_SensorsDataBuffferMap, SensorID, new SSensorInformation(BDADDRESS, SensorID, ChildrenSensorIDs)))
	{
		LogEvent(LE_FATAL, __FUNCTION__ ": Failed to add SensorID %d to Map. Configuration is Corrupt! Is there a Recursive settings??",
			SensorID);
		return false;
	}

	return BuildSensorControllerInfoBranch(SensorID);
}

void CSensorController::ReportSensorsStatusToPositionManager()
{
	std::map<int /*SensorId*/, SSensorInformation*>::iterator Iter = m_SensorsDataBuffferMap.begin();
	std::map<int /*SensorId*/, SSensorInformation*>::iterator End = m_SensorsDataBuffferMap.end();

	for(;Iter != End; ++Iter)
	{
		SSensorInformation* SensorInformation = Iter->second;
		m_EventsHandler->OnSensorInSystem(SensorInformation->m_SensorID, SensorInformation->m_SensorID == m_SensorID, SensorInformation->m_BDADDRESS, SensorInformation->m_ChildrenSensorIDs);
		SendStatusUpdate(SensorInformation);
	}

}

void CSensorController::ReadGeneralSensorsConfiguration()
{
	m_TimeBetweenConnectionAttempts = GetConfigInt(GENERAL_SENSORS_CONFIG_SECTION, "TimeBetweenConnectionAttempts", DEFAULT_TIME_BETWEEN_CONNCETION_ATTEMPTS);
	m_TimeBetweenHandshakeAttempts = GetConfigInt(GENERAL_SENSORS_CONFIG_SECTION, "TimeBetweenHandshakeAttempts", DEFAULT_TIME_BETWEEN_HANDSHAKE_ATTEMPTS);
	m_AllowedTimeBetweenKeepAlives = GetConfigInt(GENERAL_SENSORS_CONFIG_SECTION, "AllowedTimeBetweenKeepAlives", DEFAULT_TIME_BETWEEN_KEEP_ALIVES);
	m_StatusUpdateResolution = GetConfigInt(GENERAL_SENSORS_CONFIG_SECTION, "StatusUpdateResolution", DEFAULT_STATUS_UPDATE_RESOLUTION);
}

void CSensorController::DoStatusUpdateIfNecessary(DWORD TickCount)
{
	if (TickCount - m_LastStatusUpdateTickCount < m_StatusUpdateResolution)
		return;

	m_LastStatusUpdateTickCount = TickCount;

	std::map<int /*SensorId*/, SSensorInformation*>::iterator Iter = m_SensorsDataBuffferMap.begin();
	std::map<int /*SensorId*/, SSensorInformation*>::iterator End = m_SensorsDataBuffferMap.end();

	for(;Iter != End; ++Iter)
	{
		SSensorInformation *SensorInformation = Iter->second;

		SendStatusUpdate(SensorInformation);
	}
}

void CSensorController::SendStatusUpdate(SSensorInformation *SensorInformation)
{
	m_EventsHandler->OnSensorStatusUpdate(SensorInformation->m_SensorID, SensorInformation->m_SensorID == m_SensorID,
		m_PhysicalConnectionStatus, SensorInformation->m_HandshakeStatus, SensorInformation->m_LogicalConnectionStatus);
}