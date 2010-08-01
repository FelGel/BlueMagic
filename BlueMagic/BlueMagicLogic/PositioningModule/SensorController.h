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

struct SSensorDataBuffer
{
	SSensorDataBuffer() {m_DataBufferOffset = 0;};

	BYTE m_DataBuffer[DATA_BUFFER_SIZE];
	DWORD m_DataBufferOffset;
};

class CSensorController : public CThreadWithQueue, public ISerialPortEvents, ISensorCommands
{
public:
	CSensorController(int SensorID, int ComPort, std::string BDADDRESS);
	~CSensorController(void);

	bool Init(ISensorEvents *Handler);
	void Close(); // called right before it is deleted

protected:
	virtual void OnDataReceived(int SerialPortID, BYTE *Data, int DataLength);

	virtual void GetInfo();
	virtual void GetData();
	virtual void DefineTopology(/*......*/);

private:
	void HandleDataReceived(const SDataFromSensor& DataFromSensor);
	DWORD ParseData(int SensorID, BYTE *Data, int DataLength);
	bool IsHeaderValid(EBlueMagicBTBIncomingMessageType MessageType);
	CBlueMagicBTBIncomingMessage* CreateBlueMagicBTBMessage(EBlueMagicBTBIncomingMessageType MessageType);
	void CallEventOnMessage(int /*SensorID*/, const CBlueMagicBTBIncomingMessage* Message, UINT /*MessageSize*/);

	void HandleGetInfo();
	void HandleGetData();
	void HandleDefineTopology(/*......*/);

	bool ConnectToPort();
	bool SendBlueMagicMessageToSensor(const CBlueMagicBTBOutgoingMessage* Message, const int& SensorID /*= CTcpSocketServer::SEND_ALL*/);

private:
	ISensorEvents *m_EventsHandler;
	CSerialPort m_SerialPort;

	int m_SensorID;
	int m_ComPort;
	std::string m_BDADDRESS;

	std::map<int /*SensorId*/, SSensorDataBuffer*> m_SensorsDataBuffferMap;
};
