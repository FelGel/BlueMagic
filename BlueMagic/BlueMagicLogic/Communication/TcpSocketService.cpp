// TcpSocket.cpp: implementation of the CTcpSocketService class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "TcpSocketService.h"
#include "BerkeleySocketsUtils.h"
#include "Common/LogEvent.h"
#include "Common/Utils.h"
#include "Common/CriticalSection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// class CTcpSocketService

CTcpSocketService::CTcpSocketService() : 
    m_KeepAliveHandler(NULL),
    m_EnableKeepAlive(false),
    m_OutgoingFilter(NULL),
    m_IncomingFilter(NULL),
    m_LastErrorCode(0),
    m_LastErrorCodeTime(0),
    m_Sink(NULL),
    m_IsConnected(false)
{
	LogEvent(LE_INFOLOW, "CTcpSocketService constructor");
}

CTcpSocketService::~CTcpSocketService()
{
    StopTimer();
    Close();
	LogEvent(LE_INFOLOW, "CTcpSocketService Destructor");
    RemoveFilters();
    if(m_KeepAliveHandler != NULL)
    {
        delete m_KeepAliveHandler;
        m_KeepAliveHandler = NULL;
    }
    m_Sink = NULL;
}


void CTcpSocketService::SetFilters(ITcpOutgoingFilter* OutgoingFilter /* = NULL */,
                                   ITcpIncomingFilter* IncomingFilter /* = NULL */)
{

    RemoveFilters();

    m_OutgoingFilter = OutgoingFilter;
    m_IncomingFilter = IncomingFilter;

    if(m_OutgoingFilter != NULL)
        m_OutgoingFilter->AdviseOutgoingFilterEvents(this);

    if(m_IncomingFilter != NULL)
        m_IncomingFilter->AdviseIncomingFilterEvents(this);
}

bool CTcpSocketService::Create(int LocalSocketPort, const char* LocalSocketAdd,
                               ITcpSocketServiceEvents* Sink)
{
    m_Sink = Sink;
    m_IsConnected = false;
    return CBaseBerkeleySocket::Create(LocalSocketPort, LocalSocketAdd, TCP_SOCKET, this);
}

bool CTcpSocketService::Create(SOCKET ExistingSocket, bool IsSocketConnected,
                               ITcpSocketServiceEvents* Sink)
{
    m_Sink = Sink;
    m_IsConnected = IsSocketConnected;
    return CBaseBerkeleySocket::Create(ExistingSocket, this);
}

// TODO: should we wait for until it is closed. (As done in the old sockets)?
void CTcpSocketService::Close()
{
    if(m_IncomingFilter != NULL)
        m_IncomingFilter->Reset();

    if(m_OutgoingFilter != NULL)
        m_OutgoingFilter->Reset();

    CBaseBerkeleySocket::Close();
}

bool CTcpSocketService::IsConnected() const
{
    return m_IsConnected;
}

void CTcpSocketService::SetKeepAliveHandler(CBaseKeepAliveHandler* Handler)
{
    //release previous handler
    if(m_KeepAliveHandler != NULL)
    {
        StopTimer();
        m_KeepAliveHandler->Advise(NULL);
        if(m_KeepAliveHandler != NULL)
        {
            delete m_KeepAliveHandler;
            m_KeepAliveHandler = NULL;
        }
    }

    if(Handler == NULL)
        return;

    m_KeepAliveHandler = Handler;
    m_KeepAliveHandler->Advise(this);
    DWORD KeepAliveTimeoutInterval =  Handler->TimeoutFrequency();
    if(GetCurrentTimerRate() <= 0 || GetCurrentTimerRate() > KeepAliveTimeoutInterval )
        StartTimer(KeepAliveTimeoutInterval, "TcpSocketService");
}

void CTcpSocketService::EnableKeepAlive(bool Enable)
{
    //call this function according to the connection state: enable on connect and disable on disconnect
    m_EnableKeepAlive = Enable;
    if(Enable && m_KeepAliveHandler != NULL)
        m_KeepAliveHandler->Reset();
}

void CTcpSocketService::HandleSocketError(int ErrorCode, const char* ContextStr,
                                          ELogSeverity Severity)
{
    //the following code will filter the display of incoming errors 
    if(ErrorCode != m_LastErrorCode || 
        GetTickCount()- m_LastErrorCodeTime > DISPLAY_ERROR_FREQUENCY)
    {
        m_LastErrorCode = ErrorCode;
        m_LastErrorCodeTime = GetTickCount();
        DisplaySocketsError(ContextStr, Severity);
    }
}

bool CTcpSocketService::SendMessage(const void* Data, int DataSize, 
                                    bool IsPriorityMsg, 
                                    ELogSeverity WouldBlockLogLevel,
									int NumWouldBlockResends, int SleepTimeBetweenResends)
{
    if(m_OutgoingFilter != NULL)
    {
        return m_OutgoingFilter->SendMessage(Data, DataSize, IsPriorityMsg);
    }
    else
    {
        ITcpOutgoingFilterEvents::ESendStatus Status = DoSendMessage(Data, DataSize,
            WouldBlockLogLevel, NumWouldBlockResends, SleepTimeBetweenResends);
        return Status == ITcpOutgoingFilterEvents::SEND_STATUS_OK;
    }
}
    
// private methods
ITcpOutgoingFilterEvents::ESendStatus CTcpSocketService::DoSendMessage(
        const void* Data, int DataSize, 
        ELogSeverity WouldBlockLogLevel, int NumWouldBlockResends, int SleepTimeBetweenResends)
{
    //CCriticalSectionLocker SendLocker(m_SocketGuard);
    const int OriginalNumWouldBlockResends = NumWouldBlockResends;
    ITcpOutgoingFilterEvents::ESendStatus Status = ITcpOutgoingFilterEvents::SEND_STATUS_FAILED;

    int	  Sent		= 0;
    DWORD LastError = 0; // No Error
    const void* DataToSend = Data;
    int DataToSendSize = DataSize;
    while(true)
    {
        Sent = Send(DataToSend, DataToSendSize);
        if (Sent == DataToSendSize)
        {
            //if(OriginalNumWouldBlockResends > 0)
            //	LogEvent(LE_INFOHIGH, "TcpSocketService::SendMessage : success on %d attempt.", OriginalNumWouldBlockResends - NumWouldBlockResends);
            return ITcpOutgoingFilterEvents::SEND_STATUS_OK;
        }
        else if (Sent == SOCKET_ERROR)
        {
            LastError = WSAGetLastError();
            if(LastError != WSAEWOULDBLOCK)
            {
                Status = ITcpOutgoingFilterEvents::SEND_STATUS_FAILED;
                break;
            }
        }
        else // Message was only partially send
        {
            DataToSend = ((const char*) DataToSend) + Sent;
            DataToSendSize -= Sent;
        }

        //receive would block or message sent only partially - need to wait & resend
        if(--NumWouldBlockResends < 0)
        { // If the number of resends exceeded.
            Status = ITcpOutgoingFilterEvents::SEND_STATUS_BLOCKED;
            break;
        }

        //retry on would block
        Sleep(SleepTimeBetweenResends);
    }

    Assert(Sent != DataToSendSize);

    if(Status == ITcpOutgoingFilterEvents::SEND_STATUS_BLOCKED)
    {
        LogEvent(WouldBlockLogLevel, "CTcpSocketService::DoSendMessage: Socket is blocked, Send Size [%d]. Resends = [%d].", 
             DataSize, OriginalNumWouldBlockResends);
    }
    else if(Status == ITcpOutgoingFilterEvents::SEND_STATUS_FAILED)
    {
        DWORD ErrorCode = GetLastError();
        HandleSocketError(ErrorCode, "CTcpSocketService::DoSendMessage", LE_WARNING);
    }

    return Status;
}

//implement filters events
ITcpOutgoingFilterEvents::ESendStatus CTcpSocketService::OnOutgoingFilterMessage(const void* Data, int DataSize)
{
    return DoSendMessage(Data, DataSize, LE_DEBUG, 0, 0);
}

/*virtual*/ void CTcpSocketService::OnIncomingFilterMessage(const void* Data, int DataSize)
{
    SendIncomingMessage((const BYTE*)Data, DataSize);
}

// IBaseBerkeleySocketEvents
/*virtual*/ 
void CTcpSocketService::OnReceive()
{
// 	LogEvent(LE_DEBUG, "CTcpSocketService OnReceive()");

    int ActualReceived = Receive((char*)m_Data, m_DataSize);

    if(ActualReceived == SOCKET_ERROR)
    {
        DWORD LastError = GetLastError();
        if (LastError == WSAEWOULDBLOCK || LastError == WSAENOTCONN)
            LogEvent(LE_INFO, "CTcpSocketService::OnReceive() Receive Error[%s]", GetSocketErrorStr(LastError).c_str());
        else
        {
            LogEvent(LE_INFO, "CTcpSocketService::OnReceive() Receive Error, Closing the Socket. Error [%s] ", 
                    GetSocketErrorStr(LastError).c_str());
            CloseOnProblem();
        }
        return;
    }

	if(ActualReceived == BERKELEY_SOCKETS_OK)
        return; // means that there is no message received and no error (connection closed)

    if(m_IncomingFilter != NULL)
    {
        m_IncomingFilter->OnIncomingMessage(m_Data, ActualReceived);
    }
    else
    {
        LogEvent(LE_INFOLOW, "CTcpSocketService::OnReceive ( Incoming Filter is NULL)");
        SendIncomingMessage(m_Data, ActualReceived);
    }
}

void CTcpSocketService::SendIncomingMessage(const BYTE* Data, int DataSize)
{
    if (m_Sink == NULL)
    {
        LogEvent(LE_ERROR, "CTcpSocketService::SendIncomingMessage: m_Sink == NULL"); 
        return;
    }

    //try to see if its a keep alive message
    if(m_KeepAliveHandler != NULL)
    {
        if(m_KeepAliveHandler->HandleKeepAliveMessage(Data, DataSize))
            return;
    }
    
    //its not a keep alive message
    m_Sink->OnMessage(Data, DataSize);
}

void CTcpSocketService::RemoveFilters()
{
    //un advise
    if(m_OutgoingFilter != NULL)
    {
        m_OutgoingFilter->AdviseOutgoingFilterEvents(NULL);
        delete m_OutgoingFilter;
        m_OutgoingFilter = NULL;
    }

    if(m_IncomingFilter != NULL)
    {
        m_IncomingFilter->AdviseIncomingFilterEvents(NULL);
        delete m_IncomingFilter;
        m_IncomingFilter = NULL;
    }
}

void CTcpSocketService::CloseOnProblem()
{
    //call this function internally in order to close the socket in case of error situation
    EnableKeepAlive(false);
    Close();//CBaseBerkeleySocket::Close();

    if(m_Sink)
        m_Sink->OnClose();
    else
        LogEvent(LE_ERROR, "CTcpSocketService::OnBaseSocketClose: m_Sink=NULL");
}


/*virtual*/ 
void CTcpSocketService::OnSend()
{
	// Optimize keep alive handling in case the socket becomes blocked for too much
	// time by an aggressive user.
	if(NULL != m_KeepAliveHandler)
	{
		m_KeepAliveHandler->Reset();
		m_KeepAliveHandler->OnTimeout();
	}
	if(m_Sink)
		m_Sink->OnSend();
	else
		LogEvent(LE_ERROR, "CTcpSocketService::OnSend : m_Sink=NULL");
}

/*virtual*/ 
void CTcpSocketService::OnAccept()
{
	if(m_Sink)
		m_Sink->OnAccept();
	else
		LogEvent(LE_ERROR, "CTcpSocketService::OnAccept: m_Sink=NULL");
}

/*virtual*/ 
void CTcpSocketService::OnConnect(int nErrorCode)
{
    m_IsConnected = true;

	if(m_Sink)
		m_Sink->OnConnect(nErrorCode);
	else
        LogEvent(LE_ERROR, "CTcpSocketService::OnConnect: m_Sink=NULL");
}

void CTcpSocketService::OnConnectionClosed(int ErrorCode)
{
    LogEvent(LE_INFO, "CTcpSocketService::OnConnectionClosed - remote connection closed with error code[%s]. Closing socket",
                ErrorCode == BERKELEY_SOCKETS_OK ? "OK" : GetSocketErrorStr(ErrorCode).c_str());
    m_IsConnected = false;
    CloseOnProblem();
}

/*virtual*/
void CTcpSocketService::OnTimeout()
{
    if(m_KeepAliveHandler != NULL && m_EnableKeepAlive)
		m_KeepAliveHandler->OnTimeout();

    if(m_Sink)
        m_Sink->OnSocketTimer();
}

//implement IKeepAliveHandlerEvents  
void CTcpSocketService::OnKeepAliveDisconnection()
{
    LogEvent(LE_WARNING, "CTcpSocketService::OnKeepAliveDisconnection(), Close the socket and notify the Sink");
    CloseOnProblem();
}

bool CTcpSocketService::OnSendKeepAliveMsg(const BYTE* KeepAliveMsgData, int DataSize)
{
//	enum { NUM_WOULD_BLOCK_RESENDS = 3 };
    return SendMessage(KeepAliveMsgData, DataSize, true, LE_INFOLOW/*, LE_ERROR, NUM_WOULD_BLOCK_RESENDS*/);
}



bool CTcpSocketService::GetPeerData(std::string& Ip, UINT& Port)
{
    bool Success = (GetPeerName(Ip, (int&)Port) != FALSE);

    if(!Success)
        DisplaySocketsError("CTcpSocketService::GetPeerName");
    return Success;
}


bool CTcpSocketService::GetSockName(std::string& Ip, UINT& Port)
{
    bool Success = (CBaseBerkeleySocket::GetSockName(Ip, (int&)Port) != FALSE);

    if(!Success)
        DisplaySocketsError("CTcpSocketService::GetSockName");
    return Success;
}

