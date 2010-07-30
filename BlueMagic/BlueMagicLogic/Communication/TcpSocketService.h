// TcpSocketService.h: interface for the CTcpSocketService class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>
#include "BaseBerkeleySocket.h"
#include "BaseKeepAliveHandler.h"
#include "TcpSocketExtensions.h"
#include "Common/LogEvent.h"

//////////////////////////////////////////////////////////////////////////
// The CTcpSocketService is the main class for handling tcp communication.
// It inherits from the BaseSocket and allow clients to provide extension 
// capabilities, like parsing, incoming and outgoing filters and keep alive. 
// The class maintains a Data buffer of incoming data, since received data
// can contain partial client message or several messages. 
//////////////////////////////////////////////////////////////////////////
class ITcpSocketServiceEvents
{
public:
    virtual void OnMessage(const void* /* Data */, int /* DataSize */) {}
	virtual void OnSend() {}
	virtual void OnAccept() {}
	virtual void OnConnect(int /* nErrorCode */) {}
	virtual void OnClose()  {}
    virtual void OnSocketTimer() {}
};


//////////////////////////////////////////////////////////////////////////

class CTcpSocketService : public CBaseBerkeleySocket, 
    public IBaseBerkeleySocketEvents, public IKeepAliveHandlerEvents,
    public ITcpOutgoingFilterEvents, public ITcpIncomingFilterEvents
{
public:
	CTcpSocketService();
	virtual ~CTcpSocketService();

    bool Create(int LocalSocketPort, const char* LocalSocketAdd, ITcpSocketServiceEvents* Sink);
    bool Create(SOCKET ExistingSocket, bool IsSocketConnected, ITcpSocketServiceEvents* Sink);
    void Close();

    bool IsConnected() const;

    //incoming and outgoing parsers/filters - responsibility to delete filters
    //is on the client
    void SetFilters(ITcpOutgoingFilter* OutgoingFilter = NULL, 
        ITcpIncomingFilter* IncomingFilter = NULL);

    void SetKeepAliveHandler(CBaseKeepAliveHandler* Handler);
    void EnableKeepAlive(bool Enable);  //call this function according to the connection state: enable on connect and disable on disconnect

    bool SendMessage(const void* Data, int DataSize, bool IsPriorityMsg = true,
        ELogSeverity WouldBlockLogLevel = LE_ERROR, int NumWouldBlockResends = 0, int SleepTimeBetweenResends = 10);

	// These methods simply hide the use of CString
    bool GetSockName(std::string& Ip, UINT& Port);
    bool GetPeerData(std::string& Ip, UINT& Port);

    // change the accessibility of those functions
    using CBaseBerkeleySocket::Listen;
    using CBaseBerkeleySocket::Accept;
    using CBaseBerkeleySocket::Connect;

private:
    // Hide copy ctor and assignment operator
    CTcpSocketService(const CTcpSocketService &);
    CTcpSocketService & operator=(const CTcpSocketService &);

private:
    // IBaseBerkeleySocketEvents
    virtual void OnReceive();
    virtual void OnSend();
    virtual void OnAccept();
    virtual void OnConnect(int ErrorCode);
    virtual void OnConnectionClosed(int ErrorCode);

    //override virtual methods of IBaseBerkeleySocketEvents
    virtual void OnTimeout();

    //implement  IKeepAliveHandlerEvents  
    virtual void OnKeepAliveDisconnection();
    virtual bool OnSendKeepAliveMsg(const BYTE* KeepAliveMsgData, int DataSize);

    //implement ITcpOutgoingFilterEvents and ITcpIncomingFilterEvents
    virtual void OnIncomingFilterMessage(const void* Data, int DataSize);
    virtual ITcpOutgoingFilterEvents::ESendStatus OnOutgoingFilterMessage(const void* Data, int DataSize);


    //helpers
    ITcpOutgoingFilterEvents::ESendStatus DoSendMessage(
        const void* Data, int DataSize, ELogSeverity WouldBlockLogLevel = LE_ERROR,
        int NumWouldBlockResends = 0, int SleepTimeBetweenResends = 10);

    void SendIncomingMessage(const BYTE* Data, int DataSize); //send to sink
    void RemoveFilters();
    void CloseOnProblem(); //call this function internally in order to close the socket in case of error situation
    void HandleSocketError(int ErrorCode, const char* ContextStr, ELogSeverity Severity = LE_ERROR);

private:
	// data members
    ITcpSocketServiceEvents*    m_Sink;
    ITcpOutgoingFilter*         m_OutgoingFilter; 
    ITcpIncomingFilter*         m_IncomingFilter;
    bool                        m_IsConnected;

    static const int            m_DataSize = 2000;
    BYTE                        m_Data[m_DataSize];

    //keep alive parameters
    CBaseKeepAliveHandler*      m_KeepAliveHandler;
    bool                        m_EnableKeepAlive;

    //parameters for filtering log events that occur many times per seconds.
    int     m_LastErrorCode;
    DWORD   m_LastErrorCodeTime;
    enum { DISPLAY_ERROR_FREQUENCY = 2000 };
};


