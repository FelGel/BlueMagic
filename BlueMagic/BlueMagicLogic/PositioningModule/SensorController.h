#pragma once
#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"
#include "ISensorCommands.h"
#include "SerialPort.h"
#include "BlueMagicBTBMessages.h"

#define DATA_BUFFER_SIZE	10 * 1024 // 10kb

struct SDataFromSensor
{
	int SensorID;
	BYTE *Data;
	DWORD DataLength;
};

struct SClockCorrelationData
{
	SClockCorrelationData()
	{
		SensorClock = SHRT_MAX + 1;
		MatchingTickCount = 0;
	}

	int SensorClock;
	DWORD MatchingTickCount;
};

struct SSensorInformation
{
	SSensorInformation() {m_DataBufferOffset = 0;};

	BYTE m_DataBuffer[DATA_BUFFER_SIZE];
	DWORD m_DataBufferOffset;
	SClockCorrelationData m_ClockCorrelationData;
};

enum ESensorConnectionStatus
{
	SensorNotConnected,
	SensorConnected,
	SensorAttemptsAtConnection,
	SensorResettingConnection,
};

enum ESensorHandshakeStatus
{
	SensorNotHandshaked,
	SensorHandshaked,
	SensorHandshakeFailed
};

class CSensorController : public CThreadWithQueue, public ISerialPortEvents, ISensorCommands
{
public:
	CSensorController(int SensorID, int ComPort, std::string BDADDRESS);
	~CSensorController(void);

	bool Init(ISensorEvents *Handler);
	void Close(); // called right before it is deleted

	virtual void GetInfo();
	virtual void GetData();
	virtual void DefineTopology(/*......*/);

	DWORD GetTimeForSensorClock(int SensorID, int Clock); // for certain clock

protected:
	virtual void OnDataReceived(int SerialPortID, BYTE *Data, int DataLength);
	virtual void OnTimeout();

private:
	// Handle Incoming Messages from BTB
	void HandleDataReceived(const SDataFromSensor& DataFromSensor);
	DWORD ParseData(int SensorID, BYTE *Data, int DataLength);
	DWORD ParseInvalidData(int SensorID, BYTE *Data, int DataLength);
	bool IsHeaderValid(EBlueMagicBTBIncomingMessageType MessageType);
	bool IsMessageComplete(BYTE *Data, int DataLength);
	CBlueMagicBTBIncomingMessage* CreateBlueMagicBTBMessage(EBlueMagicBTBIncomingMessageType MessageType);
	void CallEventOnMessage(int /*SensorID*/, const CBlueMagicBTBIncomingMessage* Message, UINT /*MessageSize*/);

	// Handle Outgoing Message to BTB
	void HandleGetInfo();
	void HandleGetData();
	void HandleDefineTopology(/*......*/);
	bool SendBlueMagicMessageToSensor(const CBlueMagicBTBOutgoingMessage* Message, const int& SensorID /*= CTcpSocketServer::SEND_ALL*/);

	// Connection setup
	bool ConnectToPort();
	void StartConnectionRetiresMechanism();
	
	// handshake setup
	void DoHandshake();
	void OnBTBInfoMessage(CBlueMagicBTBInfoMessage *BTBInfoMessage);
	
	// Connection maintenance
	void ResetConnection();
	void OnConnectionTimedOut();
	void OnValidMessageArrived(int SensorID);
	void OnBTBKeepAliveMessage(CBlueMagicBTBKeepAliveMessage *BTBKeepAliveMessage);

	// Clocks
	void SetClockForSensor(int Clock, int SensorID);

private:
	ISensorEvents *m_EventsHandler;
	CSerialPort m_SerialPort;

	int m_SensorID;
	int m_ComPort;
	std::string m_BDADDRESS;

	ESensorConnectionStatus m_ConnectionStatus;
	DWORD m_LastConnectionAttemptTickCount;
	DWORD m_LastPacketRecievedTickCount;

	ESensorHandshakeStatus m_HandshakeStatus;
	DWORD m_LastHandshakeAttemptTickCount;

	std::map<int /*SensorId*/, SSensorInformation*> m_SensorsDataBuffferMap;
};
