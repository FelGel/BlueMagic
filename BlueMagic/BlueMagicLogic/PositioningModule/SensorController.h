#pragma once
#include "..\common\threadwithqueue.h"
#include "SensorInformation.h"
#include "ISensorEvents.h"
#include "ISensorCommands.h"
#include "SerialPort.h"
#include "BlueMagicBTBMessages.h"

struct SDataFromSensor
{
	int SensorID;
	BYTE *Data;
	DWORD DataLength;
};



class CSensorController : public CThreadWithQueue, public ISerialPortEvents, ISensorCommands
{
public:
	CSensorController(int SensorID, int ComPort, std::string BDADDRESS, std::vector<int> ChildrenSensorIDs);
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
	DWORD ParseData(BYTE *Data, int DataLength);
	DWORD ParseInvalidData(BYTE *Data, int DataLength);
	bool IsHeaderValid(EBlueMagicBTBIncomingMessageType MessageType);
	bool IsMessageComplete(BYTE *Data, int DataLength);
	CBlueMagicBTBIncomingMessage* CreateBlueMagicBTBMessage(EBlueMagicBTBIncomingMessageType MessageType);
	void CallEventOnMessage(const CBlueMagicBTBIncomingMessage* Message, UINT /*MessageSize*/);
	bool IsMessageAllowedWithoutHandshake(EBlueMagicBTBIncomingMessageType MessageType);

	// Handle Outgoing Message to BTB
	void HandleGetInfo();
	void HandleGetData();
	void HandleDefineTopology(/*......*/);
	bool SendBlueMagicMessageToSensor(const CBlueMagicBTBOutgoingMessage* Message, const int& SensorID /*= CTcpSocketServer::SEND_ALL*/);

	// Connection setup
	bool ConnectToPort();
	void StartConnectionRetiresMechanism();
	void OnBTBConnectedMessage(SSensorInformation* SensorInfo);
	
	// handshake setup
	void DoHandshake();
	void OnBTBInfoMessage(CBlueMagicBTBInfoMessage *BTBInfoMessage, SSensorInformation* SensorInfo);
	void OnBTBGetInfoMessageReturned(SSensorInformation* SensorInfo);
	void CheckHandshakeStatus(DWORD TickCount);
	ESensorHandshakeStatus GetSensorBranchHandshakeStatus(int SensorID);
	ESensorHandshakeStatus GetSensorControllerBranchHandshakeStatus();
	
	// Connection maintenance
	void ResetConnection(int SensorID);
	void OnConnectionTimedOut(SSensorInformation *SensorInformation);
	void OnValidMessageArrived(int SensorID);
	void OnBTBKeepAliveMessage(CBlueMagicBTBKeepAliveMessage *BTBKeepAliveMessage);
	void CheckPhysicalConnectionStatus(DWORD TickCount);
	void CheckLogicalConnectionStatus(DWORD TickCount);
	void SensorIsActive(SSensorInformation *SensorInformation);

	// Clocks
	void SetClockForSensor(int Clock, int SensorID);
	int  GetEstimatedCurrentClock(int LastClock, DWORD LastTickCount);
	bool IsClockConsistent(int CurrentClock, int LastClock, DWORD LastTickCount);
	void ResetClock(SSensorInformation *SensorInformation);

	// Sensor Info Service Functions 
	SSensorInformation* GetSensorControllerInfo();	
	void ReportSensorsStatusToPositionManager();
	SSensorInformation* GetSensorInfoForIncomingMessage(CBlueMagicBTBIncomingMessage* BlueMagicBTBMessage);
	SSensorInformation* GetSensorInfoByBdAddress(std::string BdAddress);

	// Status Update
	void DoStatusUpdateIfNecessary(DWORD TickCount);
	void SendStatusUpdate(SSensorInformation *SensorInformation);

	// Init related
	void ReadGeneralSensorsConfiguration();
	bool BuildSensorControllerInfoTree();
	bool BuildSensorControllerInfoBranch(int SensorID);
	bool LookupRemoteSensorInConfigurationAndParse(int SensorID);
	bool ParseRemoteSensorsConfiguration(int ObjectIndex, std::string ConfigSection);

private:
	ISensorEvents *m_EventsHandler;
	CSerialPort m_SerialPort;

	std::string m_BDADDRESS;
	int m_SensorID;
	int m_ComPort;

	ESensorConnectionStatus m_PhysicalConnectionStatus;
	DWORD m_LastConnectionAttemptTickCount;
	DWORD m_LastHandshakeAttemptTickCount; // One counter per SensorController as GetInfo is a broadcast message

	DWORD m_LastKeepAliveTestingTickCount;

	BYTE m_DataBuffer[DATA_BUFFER_SIZE];
	DWORD m_DataBufferOffset;

	std::map<int /*SensorId*/, SSensorInformation*> m_SensorsDataBuffferMap;

	DWORD m_TimeBetweenConnectionAttempts; 
	DWORD m_TimeBetweenHandshakeAttempts;
	DWORD m_AllowedTimeBetweenKeepAlives;

	DWORD m_StatusUpdateResolution;
	DWORD m_LastStatusUpdateTickCount;

	bool m_DebugFlag1;
};
