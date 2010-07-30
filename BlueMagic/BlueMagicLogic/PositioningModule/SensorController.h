#pragma once
#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"
#include "SerialPort.h"

struct SDataFromSensor
{
	int SensorID;
	BYTE *Data;
	DWORD DataLength;
};

class CSensorController : public CThreadWithQueue, public ISerialPortEvents
{
public:
	CSensorController(int SensorID, int ComPort, std::string BDADDRESS);
	~CSensorController(void);

	bool Init(ISensorEvents *Handler);
	void Close(); // called right before it is deleted

protected:
	virtual void OnDataReceived(int SerialPortID, BYTE *Data, int DataLength);

private:
	void HandleDataReceived(const SDataFromSensor& DataFromSensor);
	bool ConnectToPort();

private:
	ISensorEvents *m_EventsHandler;
	CSerialPort m_SerialPort;

	int m_SensorID;
	int m_ComPort;
	std::string m_BDADDRESS;
};
