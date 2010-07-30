#pragma once

#include "serialex.h"
#include <string>

class ISerialPortEvents
{
public:
	virtual void OnDataReceived(int SerialPortID, BYTE *Data, int DataLength) = 0;
};

class CSerialPort :	public CSerialEx
{
public:
	CSerialPort(ISerialPortEvents *SerialPortEvents, int ComPort, int SerialPortID);
	~CSerialPort(void);

	bool ConnectToPort();

	static std::string ComEventToString(EEvent eEvent);
	static std::string ComErrorToString(EError eError);

protected:	
	virtual void OnEvent (EEvent eEvent, EError eError);

private:
	ISerialPortEvents *m_SerialPortEvents;
	std::string m_ComPort;
	int m_SerialPortID;
};

