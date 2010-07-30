#include "StdAfx.h"
#include "SensorController.h"

#define SENSOR_CONTROLLER_QUEUE_SIZE 10000
#define SENSOR_CONTROLLER_THREAD_TIMEOUT 100 //milisec

CSensorController::CSensorController(int SensorID, int ComPort, std::string BDADDRESS) 
	: CThreadWithQueue("SensorController", SENSOR_CONTROLLER_QUEUE_SIZE), m_SerialPort(this, ComPort, SensorID),
		m_SensorID(SensorID), m_ComPort(ComPort), m_BDADDRESS(BDADDRESS)
{
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
	LogEvent(LE_DEBUG, "CSensorController::OnDataReceived: SensorID=%d DataLength=%d, Data=%s", 
		DataFromSensor.SensorID, DataFromSensor.DataLength, DataFromSensor.Data);

	delete[] DataFromSensor.Data;
}