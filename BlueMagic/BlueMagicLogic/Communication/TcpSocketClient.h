// CTcpSocketClient.h: interface for the CTcpSocketClient class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/TimerManager.h"
#include "TcpSocketService.h"


#include <string>
#include <vector>
#include <afxsock.h>


class ITcpSocketClientEvents
{
public:
    virtual void OnMessageFromServer(const void* Data, int DataSize) = 0;
    virtual void OnConnectedToServer() = 0;
    virtual void OnDisconnectedFromServer() = 0;
    virtual void OnClientSocketTimeout() = 0;
	virtual void OnReadyForSend() {}
};


class CTcpSocketClient : 
	public ITcpSocketServiceEvents
{
public:
    static const int DefaultReconnectionTime = 5;

	CTcpSocketClient();
	virtual ~CTcpSocketClient();

    void Init(const char* ContextName, ITcpSocketClientEvents* Sink, 
        ITcpIncomingFilter* IncomingFilter = NULL,
        ITcpOutgoingFilter* OutgoingFilter = NULL,
        CBaseKeepAliveHandler* KeepAliveHandler = NULL);

    bool Create(const char* ServerIpAddress, int ServerPort,
        const char* LocalIp = NULL,  int LocalPort = 0,
        int ReconnectItervalInSec = DefaultReconnectionTime, bool UseAutoReconnect = true);

    bool SendMessage(const void* Data, int DataSize, bool IsPriorityMsg = true,
		ELogSeverity WouldBlockLogLevel = LE_ERROR, int NumWouldBlockResends = 0);

	void CloseConnection();
	bool IsConnected() const;
	bool CloseAndCreateSocket(bool UseAutoReconnect, bool ReconnectNow);
    bool StartTimer(UINT TimerRate);
    bool StopTimer();

    bool GetPeerData(std::string& Ip, UINT& Port);
    bool GetSockName(std::string& Ip, UINT& Port);
    void SetAutoReconnection(bool AutoReconnect);

    int GetLastError() const { return m_Socket.GetLastError(); }
    const char * GetLastErrorAsString() const { return m_Socket.GetLastErrorAsString(); }

    bool SetSockOptMaxMessageSize(unsigned int MaxMsgSize) { return m_Socket.SetSockOptMaxMessageSize(MaxMsgSize); }
    bool GetSockOptMaxMessageSize(unsigned int& MaxMsgSize) const { return m_Socket.GetSockOptMaxMessageSize(MaxMsgSize); }
    bool SetSockOptNoDelay(bool NoDelay) { return m_Socket.SetSockOptNoDelay(NoDelay); }
    bool GetSockOptNoDelay(bool& NoDelay) const { return m_Socket.GetSockOptNoDelay(NoDelay); }
    bool SetSockOptReceiveBuffSize(int Size) { return m_Socket.SetSockOptReceiveBuffSize(Size); }
    bool GetSockOptReceiveBuffSize(int& Size) const { return m_Socket.GetSockOptReceiveBuffSize(Size); }
    bool SetSockOptSendBuffSize(int Size) { return m_Socket.SetSockOptSendBuffSize(Size); }
    bool GetSockOptSendBuffSize(int& Size) const { return m_Socket.GetSockOptSendBuffSize(Size); }
    bool SetSockOptTos(int Tos) { return m_Socket.SetSockOptTos(Tos); }
    bool GetSockOptTos(int& Tos) const { return m_Socket.GetSockOptTos(Tos); }

private:
    //The MaintainConnectionStatus function will notify the sink regarding the connection state. In addition
    //it will recreate the socket in case that the socket is disconnected and working in AutoReconnect mode.
    //The function will try to connect immediately if there is a need to recreate the socket and 
    //the ReconnectNow flag is true.
	void MaintainConnectionStatus(bool IsConnected, bool ReconnectNow);
    bool RecreateSocket(bool Reconnect);
    void Connect();

private:
	// ITcpSocketSerivceEvents
    virtual void OnMessage(const void* Data, int DataSize);
	virtual void OnClose();
	virtual void OnConnect(int nErrorCode);
	virtual void OnSend();
	virtual void OnSocketTimer();

    bool    m_AutoReconnect;
	CTcpSocketService m_Socket;
    std::string m_ContextName;

	ITcpSocketClientEvents*   m_Sink;

    std::string         m_ServerIpAddress; // Only relevant for client
    int                 m_ServerPort;
    std::string         m_LocalIpAddress; 
    int                 m_LocalPort;
	
	bool m_IsCreated;
	bool m_IsConnected;
    int m_ConnectRetries;

	time_t	m_LastConnectionTryTime;
	int		m_ReconnectItervalInSec;
	int     m_timerID;
    DWORD   m_SinkTimerRate;   // client/derived timer rate (in milli)
    DWORD   m_ReconnectTimerRate;   //internal timer rate use for checking need for reconnection (in milli)
    DWORD   m_LastSinkTimeout; 
//    CCriticalSection m_ConnectLock;
};
