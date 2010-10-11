#include "StdAfx.h"
#include "SerialPort.h"
#include "Common/Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BUFFER_SIZE 1024
BYTE Buffer[BUFFER_SIZE];

CSerialPort::CSerialPort(ISerialPortEvents *SerialPortEvents, int ComPort, int SerialPortID) 
	: m_SerialPortEvents(SerialPortEvents), m_SerialPortID(SerialPortID)
{
	CString ComPortString;
	ComPortString.Format("%s%d", "COM", ComPort);

	m_ComPort = ComPortString;
}

CSerialPort::~CSerialPort(void)
{
}

/*virtual*/ void CSerialPort::OnEvent (EEvent eEvent, EError eError)
{
	LogEvent(LE_DEBUG, "CSerialPort::OnEvent: Event=%s, Error=%s", 
		ComEventToString(eEvent).c_str(), ComErrorToString(eError).c_str());

	if (eError == EErrorUnknown)
	{
		switch(eEvent)
		{
			case EEventRecv:
				{
					DWORD dwBytesRead = 0;
					do
					{
						LONG lLastError = Read(Buffer,BUFFER_SIZE,&dwBytesRead);
						if (lLastError != ERROR_SUCCESS)
						{
							LogEvent(LE_ERROR, __FUNCTION__ ": Failed to read Buffer. Error=%d",
								GetLastError());
							return;
						}

						if (dwBytesRead == 0)
						{
							LogEvent(LE_INFOLOW, __FUNCTION__ ": Read 0 bytes from Port");
							return;
						}

						BYTE *Data = new BYTE[dwBytesRead];
						memcpy(Data, Buffer, dwBytesRead);
						m_SerialPortEvents->OnDataReceived(m_SerialPortID, Data, dwBytesRead);
					} while (dwBytesRead == BUFFER_SIZE);
					return;
				}

			default:
				;// Do Nothing, and reach WARNING bellow.
		}
	}

	LogEvent(LE_WARNING, "CSerialPort::OnEvent: Unhandled COM Event!! Event=%s, Error=%s", 
		ComEventToString(eEvent).c_str(), ComErrorToString(eError).c_str());
}


std::string CSerialPort::ComEventToString(EEvent eEvent)
{
	switch(eEvent)
	{
		RETURN_TYPE_STR(EEventUnknown);
		RETURN_TYPE_STR(EEventNone);
		RETURN_TYPE_STR(EEventBreak);
		RETURN_TYPE_STR(EEventCTS);
		RETURN_TYPE_STR(EEventDSR);
		RETURN_TYPE_STR(EEventError);
		RETURN_TYPE_STR(EEventRing);
		RETURN_TYPE_STR(EEventRLSD);
		RETURN_TYPE_STR(EEventRecv);
		RETURN_TYPE_STR(EEventRcvEv);
		RETURN_TYPE_STR(EEventSend);
		RETURN_TYPE_STR(EEventPrinterError);
		RETURN_TYPE_STR(EEventRx80Full);
		RETURN_TYPE_STR(EEventProviderEvt1);
		RETURN_TYPE_STR(EEventProviderEvt2);
	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown EventId %d", eEvent);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}

std::string CSerialPort::ComErrorToString(EError eError)
{
	switch(eError)
	{
		RETURN_TYPE_STR(EErrorUnknown);
		RETURN_TYPE_STR(EErrorBreak);
		RETURN_TYPE_STR(EErrorFrame);
		RETURN_TYPE_STR(EErrorIOE);
		RETURN_TYPE_STR(EErrorMode);
		RETURN_TYPE_STR(EErrorOverrun);
		RETURN_TYPE_STR(EErrorRxOver);
		RETURN_TYPE_STR(EErrorParity);
		RETURN_TYPE_STR(EErrorTxFull);
	default:
		char TmpStr[64];
		sprintf_s(TmpStr, "Unknown Comm Error %d", eError);
		LogEvent(LE_WARNING, __FUNCTION__ ": %s", TmpStr);
		return TmpStr;
	}
}

bool CSerialPort::ConnectToPort()
{
	LONG    lLastError = ERROR_SUCCESS;

	// Attempt to open the serial port
	lLastError = Open(_T(m_ComPort.c_str()),0,0,true);
	if (lLastError != ERROR_SUCCESS)
	{
		LogEvent(LE_ERROR, "Failed to open ComPort %s. Error=%d", 
			m_ComPort.c_str(), GetLastError());
		return false;
	}

	// Setup the serial port
	lLastError = Setup(CSerial::EBaud115200,CSerial::EData8,CSerial::EParNone,CSerial::EStop1);
	if (lLastError != ERROR_SUCCESS)
	{
		LogEvent(LE_ERROR, "Failed to Setup ComPort %s. Error=%d", 
			m_ComPort.c_str(), GetLastError());
		return false;
	}

	// Setup handshaking
	lLastError = SetupHandshaking(CSerial::EHandshakeHardware);
	if (lLastError != ERROR_SUCCESS)
	{
		LogEvent(LE_ERROR, "Failed to SetupHandshaking ComPort %s. Error=%d", 
			m_ComPort.c_str(), GetLastError());
		return false;
	}

	LogEvent(LE_INFO, "Connected successfully to ComPort %s", 
		m_ComPort.c_str());

	return true;
}


bool CSerialPort::SendData(const BYTE *Data, DWORD DataLength)
{
	DWORD dwSentData = 0;
	LONG lLastError = Write(Data, DataLength, &dwSentData);
	
	if (lLastError != ERROR_SUCCESS)
	{
		LogEvent(LE_ERROR, "Failed to Send Data (length=%d) ComPort %s. Error=%d", 
			DataLength, m_ComPort.c_str(), GetLastError());
		return false;
	}

	if (DataLength != dwSentData)
	{
		LogEvent(LE_WARNING, "Attempted to send data in length %d on ComPort %s, yet only %d bytes were sent !!", 
			DataLength, m_ComPort.c_str(), dwSentData);
	}

	return true;
}