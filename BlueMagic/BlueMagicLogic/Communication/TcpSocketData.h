// TcpSocketData.h: interface for the CTcpSocketData class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TcpSocketService.h"

class ITcpSocketDataEvents
{
public:
    virtual void OnSocketDataMessage(const void* Data, int DataSize, int ConnectionHandle) = 0;
    virtual void OnSocketDataDisconnected(int ConnectionHandle) = 0;
	virtual void OnReadyForSend() {}
};


class CTcpSocketData :
	public ITcpSocketServiceEvents 
{
public:
	CTcpSocketData();
	virtual ~CTcpSocketData();

    //Notice: the data socket is the owner of the filters and keep alive handler.
    //It passes these objects to the TcpSocketService data member.
	void Init(int ConnectionHandle, ITcpSocketDataEvents *Sink, 
		SOCKET ConnectedSocket,	
        ITcpIncomingFilter* IncomingFilter = NULL,
        ITcpOutgoingFilter* OutgoingFilter = NULL,
        CBaseKeepAliveHandler* KeepAliveHandler = NULL);
	// should we remove the Advise from Init()??
	void Advise(ITcpSocketDataEvents* Sink);

    bool IsConnected() const;

	bool SendMessage(const void* Data, int DataSize, bool IsPriorityMsg = true,
		ELogSeverity WouldBlockLogLevel = LE_ERROR, int NumWouldBlockResends = 0);

	void CloseDataConnection();
	bool GetPeerData(std::string& Ip, UINT& Port);
	bool GetSockName(std::string& Ip, UINT& Port);

    //call this function during the accept() function
    void SetContextName(const char* ContextName) { m_Context = ContextName; }

private:
	// ITcpSocketSerivceEvents
    virtual void OnMessage(const void* Data, int DataSize);
	virtual void OnClose();
	virtual void OnSend();

	CTcpSocketService       m_Socket;
	int                     m_ConnectionHandle;
	ITcpSocketDataEvents*   m_Sink;
    std::string             m_Context;
};

