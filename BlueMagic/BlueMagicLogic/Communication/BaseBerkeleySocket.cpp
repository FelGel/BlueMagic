#include "StdAfx.h"
#include "BaseBerkeleySocket.h"
#include "BerkeleySocketsUtils.h"
#include "Common/LogEvent.h"
#include "Common/Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//CSocketEventsManager CBaseBerkeleySocket::g_SocketEventsMgr;

//////////////////////////////////////////////////////////////////////////
CBaseBerkeleySocket::CBaseBerkeleySocket() :
m_Socket(INVALID_SOCKET)
, m_IsListening(false)
, m_IsConnecting(false)
, m_IsLastSendWouldBlock(false)
, m_Sink(NULL)
, m_TimerRateInMilli(0)
, m_TimerId(NULL_TIMER_ID)
, m_LastDwordAddress(0)
{
    //Dummy call to check if library was initialized and if not initialize it
    CSocketEventsManager::GetTheSocketEventsManager();
}


//////////////////////////////////////////////////////////////////////////
CBaseBerkeleySocket::~CBaseBerkeleySocket()
{
	if (!IsClosed())
		Close();
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::Create(int LocalSocketPort, const char* LocalSocketAdd,
								 ESocketType SocketType, IBaseBerkeleySocketEvents* Sink)
{
	if (!IsClosed())
		Close();

	// Create the socket
	int Type = SocketType == TCP_SOCKET ? SOCK_STREAM : SOCK_DGRAM;
	m_Socket = socket(AF_INET, Type, 0);
	if (m_Socket == INVALID_SOCKET)
	{
		DisplaySocketsError("CBaseBerkeleySocket::Create", LE_ERROR);
		return false;
	}

    // TODO: should I bind? only in TCP?
    // Bind socket to the port (only is the port is valid - otherwise don't do bind)
   if (SocketType == UDP_SOCKET || LocalSocketPort > 0)
    {
        if (Bind(LocalSocketPort, LocalSocketAdd) == false)
        {
            LogEvent(LE_ERROR, "CBaseBerkeleySocket::Create - Bind Error on socket [%x]: %s", m_Socket,
                GetSocketErrorStr(WSAGetLastError()).c_str());
            goto CBaseBerkeleySocket_Create_CloseAndExit;
        }
    }

	// set the socket to be non blocking - since all operation should be non blocking
	unsigned long NonBlocking = 1; // Some positive value
	if (ioctlsocket(m_Socket, FIONBIO, &NonBlocking) == SOCKET_ERROR)
	{
        LogEvent(LE_ERROR, "CBaseBerkeleySocket::Create - Error set non blocking mode to socket [%x]: %s", m_Socket,
			GetSocketErrorStr(WSAGetLastError()).c_str());
		goto CBaseBerkeleySocket_Create_CloseAndExit;
	}

    // Init the parameters
    m_Sink = Sink;
    m_IsListening = false;
    m_IsConnecting = false;
    m_IsLastSendWouldBlock = false;

	// Register this socket to the events
    CSocketEventsManager* EventMgr = CSocketEventsManager::GetTheSocketEventsManager();
    if (EventMgr == NULL || EventMgr->RegisterSocket(m_Socket, this) == false)
	{
		LogEvent(LE_ERROR, "CBaseBerkeleySocket::Create - can't register socket [%x] to socket events manager", m_Socket);
		Assert(false);
		goto CBaseBerkeleySocket_Create_CloseAndExit;
	}

    LogEvent(LE_INFO, "CBaseBerkeleySocket::Create - socket [%x] created and registered successfully", m_Socket);
	return true;

CBaseBerkeleySocket_Create_CloseAndExit:
	Close();
	m_Socket = INVALID_SOCKET;
	return false;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::Create(SOCKET AlreadyCreatedSocket, IBaseBerkeleySocketEvents* Sink)
{
    if (!IsClosed())
        Close();

    // Init the parameters
    m_Socket = AlreadyCreatedSocket;
    m_Sink = Sink;
    m_IsListening = false;
    m_IsConnecting = false;
    m_IsLastSendWouldBlock = false;

    // Register this socket to the events
    CSocketEventsManager* EventMgr = CSocketEventsManager::GetTheSocketEventsManager();
    if (EventMgr == NULL || EventMgr->RegisterSocket(m_Socket, this) == false)
    {
        LogEvent(LE_ERROR, "CBaseBerkeleySocket::Create - can't register socket [%x] to socket events manager", m_Socket);
        Assert(false);
        Close();
        m_Socket = INVALID_SOCKET;
        return false;
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
void CBaseBerkeleySocket::Close()
{
	if (IsClosed())
		return;

    CSocketEventsManager* EventMgr = CSocketEventsManager::GetTheSocketEventsManager();
    if (EventMgr == NULL)
        LogEvent(LE_WARNING, "CBaseBerkeleySocket::Close - The SocketEventsManager is already closed (Socket [%x])", m_Socket);
    else
        EventMgr->UnregisterSocket(m_Socket);

    ShutDown();

    if (closesocket(m_Socket) != BERKELEY_SOCKETS_OK)
    {
        LogEvent(LE_ERROR, "CBaseBerkeleySocket::Close - received error in CloseSocket command. socket [%x], message - %s",
            m_Socket, GetSocketErrorStr(WSAGetLastError()).c_str());
    }

    LogEvent(LE_INFO, "CBaseBerkeleySocket::Close - socket [%x] was closed", m_Socket);

	m_Socket = INVALID_SOCKET;
	return;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::IsClosed() const
{
	return m_Socket == INVALID_SOCKET;
}


#define CHECK_SOCKET_CREATED(Func) \
	if (IsClosed())   \
		{ LogEvent(LE_WARNING, #Func " socket [%x] is closed", m_Socket); \
		  return false; }	


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::Connect(const char* HostAddress, int HostPort)
{
	CHECK_SOCKET_CREATED("CBaseBerkeleySocket::Connect");

    if (m_IsConnecting)
    {
        LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::Connect - received connect request while already trying to connect. continue waiting for response. Socket [%x]",
                    m_Socket);
        return true;
    }

	struct sockaddr_in SockAddress;
	SockAddress.sin_family      = AF_INET;
	SockAddress.sin_addr.s_addr = CalculateAdressFromIp(HostAddress);
	SockAddress.sin_port        = htons((WORD)HostPort);

    // Setting this status before the call to connect in order to be sure
    // that no return value from connect(on different thread) will be returned
    // before setting this variable
    SetIsConnectingFlag(true);

	if (connect(m_Socket, (SOCKADDR*)&SockAddress, sizeof(SockAddress)) != BERKELEY_SOCKETS_OK)
	{
        int LastError = WSAGetLastError();
		if (LastError != WSAEWOULDBLOCK && LastError != WSAEINPROGRESS && LastError != WSAEALREADY)
		{
			LogEvent(LE_WARNING, "CBaseBerkeleySocket::Connect - received error while trying to connect. Socket [%x], IP [%s (%s)], port [%d], message - %s",
						m_Socket, GetStandardIpFromDWORD(SockAddress.sin_addr.s_addr).c_str(), HostAddress, HostPort, GetConnectErrorStr(WSAGetLastError()).c_str());
            SetIsConnectingFlag(false);
			return false;
		}
	}

    LogEvent(LE_INFO, "CBaseBerkeleySocket::Connect - socket [%x] connecting to IP [%s (%s)] port [%d]",
                        m_Socket, GetStandardIpFromDWORD(SockAddress.sin_addr.s_addr).c_str(), HostAddress, HostPort);

	return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::Listen()
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::Listen");

    // Setting this variable before the call to listen in order to be sure
    // that no return value from listen(on different thread) will be returned
    // before setting this variable
    m_IsListening = true;

    if (listen(m_Socket, SOMAXCONN) != BERKELEY_SOCKETS_OK)
    {
        LogEvent(LE_WARNING, "CBaseBerkeleySocket::Listen - received error in listen command. Socket [%x], message - %s",
            m_Socket, GetSocketErrorStr(WSAGetLastError()).c_str());
        m_IsListening = false;
        return false;
    }

    LogEvent(LE_INFO, "CBaseBerkeleySocket::Listen - socket [%x] start listening", m_Socket);

    return true;
}


//////////////////////////////////////////////////////////////////////////
SOCKET CBaseBerkeleySocket::Accept(bool ShouldContinueListening)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::Accept");

    struct sockaddr_in SockAddress;
    int AddressSize = sizeof(SockAddress);

    SOCKET NewSocket = accept(m_Socket, (SOCKADDR*)&SockAddress, &AddressSize);
    if (NewSocket == INVALID_SOCKET)
    {
        LogEvent(LE_WARNING, "CBaseBerkeleySocket::Accept - received error in accept command. Socket [%x], message - %s",
                    m_Socket, GetSocketErrorStr(WSAGetLastError()).c_str());
        return INVALID_SOCKET;
    }

    LogEvent(LE_INFO, "CBaseBerkeleySocket::Accept [%x]: accept NewSocket = %x", m_Socket, NewSocket);

    if (ShouldContinueListening)
        Listen();

    return NewSocket;
}


//////////////////////////////////////////////////////////////////////////
int CBaseBerkeleySocket::Send(const void* Buffer, int BufferLen)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::Send");

    int Actual = send(m_Socket, (char*)Buffer, BufferLen, 0);
    //if (Actual != BufferLen)
        CheckingWouldBlockOnSend(Actual);

    return Actual;
}


//////////////////////////////////////////////////////////////////////////
int CBaseBerkeleySocket::SendTo(const void* Buffer, int BufferLen,
                                const char* HostAddress, int HostPort)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::SendTo");

    struct sockaddr_in SockAddress;
    SockAddress.sin_family      = AF_INET;
    SockAddress.sin_addr.s_addr = CalculateAdressFromIp(HostAddress);
    SockAddress.sin_port        = htons((WORD)HostPort);

    int Actual = sendto(m_Socket, (char*)Buffer, BufferLen, 0,
                        (SOCKADDR*)&SockAddress, sizeof SockAddress);
    //if (Actual != BufferLen)
        CheckingWouldBlockOnSend(Actual);

    return Actual;
}


//////////////////////////////////////////////////////////////////////////
//Receives data from the socket. In this function it should deal with the possibility
//of a disconnection by the remote side. It all depends on the return value of the
//receive() function of the Windows Socket library:
// o	If it is a positive number ? meaning the bytes that has being received. Everything is OK
// o	If it is 0 ? It means that the socket was disconnected gracefully from the remote side.
//      Trigger the OnConnectionClosed() event with OK error code.
// o	If it is indicating error and the error is would block ? No data is currently available,
//      try again later. This is basically shouldn?t happen if the client of this class is calling
//      this function only after OnReceive() event.
// o	If it is indicating error and the error is connection reset ? It means that the
//      connection was terminated and not gracefully. Trigger the OnConnectionClosed() event
//      with the error code. Note that on UDP this means that the port is unreachable.
// o	If it is indicating error and the error is other error -  Not clear whether it should
//      trigger the OnConnectionClosed() event or not.
//
// In any case of error this function will return SOCKET_ERROR, otherwise
// the num of bytes received or 0 if no bytes received but there is no error
int CBaseBerkeleySocket::Receive(char* Buffer, int BufferLen)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::Receive");

    int ActualReceived = recv(m_Socket, Buffer, BufferLen, 0);
    if (ActualReceived > 0)
        return ActualReceived;
    else if (ActualReceived == 0)
    { // Gracefully disconnected from the remote side
        LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::Receive - connection was terminated gracefully for socket [%x]", m_Socket);
        if (m_Sink != NULL)
            m_Sink->OnConnectionClosed(BERKELEY_SOCKETS_OK);
        return BERKELEY_SOCKETS_OK;
    }
    else // == SOCKET_ERROR
    {
        int Error = WSAGetLastError();
        if (Error == WSAECONNRESET)
        { // Disconnected but not gracefully
            LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::Receive - connection was reset(not gracefully) for socket [%x]", m_Socket);
            if (m_Sink != NULL)
                m_Sink->OnConnectionClosed(Error);
            return BERKELEY_SOCKETS_OK;
        }
        else
            return SOCKET_ERROR; // nothing to add.
    }
}


//////////////////////////////////////////////////////////////////////////
// return the number of bytes read or SOCKET_ERROR if there is error.
int CBaseBerkeleySocket::ReceiveFrom(char* Buffer, int BufferLen,
                                     std::string& RemoteIP, int& RemotePort)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::ReceiveFrom");

    struct sockaddr_in RemoteAddress;
    int RemoteAddressSize = (int)sizeof(RemoteAddress);

    int Actual = recvfrom(m_Socket, Buffer, BufferLen, 0, 
                        (SOCKADDR*)&RemoteAddress, &RemoteAddressSize);

    // Not using inet_ntoa since its reset the last error so it is impossible
    // afterwards to know which error occurred in case of SOCKET_ERROR
    //RemoteIP = inet_ntoa(RemoteAddress.sin_addr);
    DWORD Address = RemoteAddress.sin_addr.s_addr;
    RemoteIP = GetStandardIpFromDWORD(Address);

    RemotePort = (int)RemoteAddress.sin_port;

    return Actual;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::GetPeerName(std::string& PeerIP, int& PeerPort) const
{
    if (m_PeerIp.empty())
    {
        CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetPeerName");

        struct sockaddr_in Peer;
        int PeerLen = sizeof(Peer);
        //Ask getpeername to fill in peer's socket address.
        if (getpeername(m_Socket, (SOCKADDR*)&Peer, &PeerLen) != BERKELEY_SOCKETS_OK)
        {
            LogEvent(LE_WARNING, "CBaseBerkeleySocket::GetPeerName - received error in getting peer name. Socket [%x], message - %s",
                m_Socket, GetSocketErrorStr(WSAGetLastError()).c_str());
            return false;
        }
        m_PeerIp = inet_ntoa(Peer.sin_addr);
        m_PeerPort = (int)ntohs(Peer.sin_port);
    }

    PeerIP = m_PeerIp;
    PeerPort = m_PeerPort;
    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::GetSockName(std::string& SockIP, int& SockPort) const
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetSockName");

    struct sockaddr_in Socket;
    int ParamLen = sizeof(Socket);
    //Ask getpeername to fill in peer's socket address.
    if (getsockname(m_Socket, (SOCKADDR*)&Socket, &ParamLen) != BERKELEY_SOCKETS_OK)
    {
        LogEvent(LE_WARNING, "CBaseBerkeleySocket::GetSockName - received error in getting peer name. Socket [%x], message - %s",
            m_Socket, GetSocketErrorStr(WSAGetLastError()).c_str());
        return false;
    }

    SockIP = inet_ntoa(Socket.sin_addr);
    SockPort = (int)ntohs(Socket.sin_port);
    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::SetSockOptMaxMessageSize(unsigned int MaxMsgSize)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::SetSockOptMaxMessageSize");

    if (setsockopt(m_Socket, SOL_SOCKET, SO_MAX_MSG_SIZE,
        (char *)&MaxMsgSize, sizeof(MaxMsgSize)) != BERKELEY_SOCKETS_OK)
        return false;

    LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::SetSockOptNoDelay - the option SO_MAX_MSG_SIZE was set to socket [%x] with value [%d]",
        m_Socket, MaxMsgSize);
    return true;
}

bool CBaseBerkeleySocket::GetSockOptMaxMessageSize(unsigned int& MaxMsgSize) const
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetSockOptMaxMessageSize");

    int ParamSize = sizeof(MaxMsgSize);
    int Status = getsockopt(m_Socket, SOL_SOCKET, SO_MAX_MSG_SIZE,
        (char*)&MaxMsgSize, &ParamSize);
    if (Status  != BERKELEY_SOCKETS_OK)
        return false;

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::SetSockOptNoDelay(bool NoDelay)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::SetSockOptNoDelay");

    BOOL Value = NoDelay? TRUE : FALSE;
    if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY,
            (char *)&Value, sizeof(Value)) != BERKELEY_SOCKETS_OK)
        return false;

    LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::SetSockOptNoDelay - the option TCP_NODELAY was set to socket [%x] with value [%d]",
                m_Socket, Value);
    return true;
}

bool CBaseBerkeleySocket::GetSockOptNoDelay(bool& NoDelay) const
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetSockOptNoDelay");

    BOOL Value;
    int ParamSize = sizeof(Value);
    int Status = getsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY,
        (char*)&Value, &ParamSize);
    if (Status  != BERKELEY_SOCKETS_OK)
        return false;

    NoDelay = !!Value;

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::SetSockOptReceiveBuffSize(int Size)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::SetSockOptReceiveBuffSize");

    if (setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF,
        (char*)&Size, sizeof(Size)) != BERKELEY_SOCKETS_OK)
        return false;

    LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::SetSockOptReceiveBuffSize - the option SO_RCVBUF was set to socket [%x] with value [%d]",
                    m_Socket, Size);
    return true;
}

bool CBaseBerkeleySocket::GetSockOptReceiveBuffSize(int& Size) const
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetSockOptReceiveBuffSize");

    int ParamSize = sizeof(Size);
    int Status = getsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF,
        (char*)&Size, &ParamSize);
    if (Status  != BERKELEY_SOCKETS_OK)
        return false;

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::SetSockOptSendBuffSize(int Size)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::SetSockOptSendBuffSize");

    if (setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF,
        (char*)&Size, sizeof(Size)) != BERKELEY_SOCKETS_OK)
        return false;

    LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::SetSockOptSendBuffSize - the option SO_SNDBUF was set to socket [%x] with value [%d]",
        m_Socket, Size);
    return true;
}

bool CBaseBerkeleySocket::GetSockOptSendBuffSize(int& Size) const
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetSockOptSendBuffSize");

    int ParamSize = sizeof(Size);
    int Status = getsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF,
        (char*)&Size, &ParamSize);
    if (Status  != BERKELEY_SOCKETS_OK)
        return false;

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::SetSockOptTos(int Tos)
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::SetSockOptTos");

    if (setsockopt(m_Socket, IPPROTO_IP, IP_TOS,
                  (char*)&Tos, sizeof(Tos)) != BERKELEY_SOCKETS_OK)
        return false;

    LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::SetSockOptTos - the option IP_TOS was set to socket [%x] with value [%d]",
                m_Socket, Tos);
    return true;
}

bool CBaseBerkeleySocket::GetSockOptTos(int& Tos) const
{
    CHECK_SOCKET_CREATED("CBaseBerkeleySocket::GetSockOptTos");

    int ParamSize = sizeof(Tos);
    int Status = getsockopt(m_Socket, IPPROTO_IP, IP_TOS,
        (char*)&Tos, &ParamSize);
    if (Status  != BERKELEY_SOCKETS_OK)
        return false;

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::StartTimer(DWORD TimerRateInMilli, const char* Context)
{
    if (m_TimerId != NULL_TIMER_ID)
        StopTimer();

    CSocketEventsManager* EventMgr = CSocketEventsManager::GetTheSocketEventsManager();
    if (EventMgr == NULL)
    {
        LogEvent(LE_WARNING, "CBaseBerkeleySocket::StartTimer - socket event thread is close. no timer. socket [%x]", m_Socket);
        return false;
    }

    if (EventMgr->AddTimer(this, TimerRateInMilli, m_TimerId, Context) == false)
    {
        LogEvent(LE_WARNING, "CBaseBerkeleySocket::StartTimer - failed to add timer for socket [%x]", m_Socket);
        return false;
    }

    m_TimerRateInMilli = TimerRateInMilli;
    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::StopTimer()
{
    if (m_TimerId == NULL_TIMER_ID)
        return true; // no timer to stop.

    CSocketEventsManager* EventMgr = CSocketEventsManager::GetTheSocketEventsManager();
    if (EventMgr == NULL)
    {
        LogEvent(LE_INFO, "CBaseBerkeleySocket::StopTimer - socket event thread is close. Timer already stopped. socket [%x]", m_Socket);
        return false;
    }

    if (EventMgr->DeleteTimer(m_TimerId) == false)
    {
        LogEvent(LE_ERROR, "CBaseBerkeleySocket::StopTimer - failed to stop timer id [%d] for socket [%x]",
                    m_TimerId, m_Socket);
        return false;
    }

    m_TimerId = NULL_TIMER_ID;
    m_TimerRateInMilli = 0;
    return true;
}


//////////////////////////////////////////////////////////////////////////
DWORD CBaseBerkeleySocket::GetCurrentTimerRate() const
{
    return m_TimerRateInMilli;
}


// TODO: it is still not clear to me whether to do shutdown on a socket before its closure
// without waiting to FD_CLOSE(?), is sufficient to gracefully closes the socket
//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::ShutDown()
{
    if (shutdown(m_Socket, SD_BOTH) != BERKELEY_SOCKETS_OK)
    {
        int LastError = WSAGetLastError();
        if (LastError != WSAENOTCONN)
        { //TODO: need to know beforehand is the socket is connected or not and 
            //not to call shutdown in case it is not connected
            LogEvent(LE_WARNING, "CBaseBerkeleySocket::ShutDown - error shutting down. Socket [%x], message - %s",
                        m_Socket, GetSocketErrorStr(LastError).c_str());
            return false;
        }
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CBaseBerkeleySocket::Bind(int LocalSocketPort, const char* LocalSocketAdd)
{
    struct sockaddr_in SockAddress;

    SockAddress.sin_family      = AF_INET;
    if (LocalSocketAdd != NULL && strcmp(LocalSocketAdd, "") != 0)
        SockAddress.sin_addr.s_addr = GetHostIPAsDWORD(LocalSocketAdd);
    else
        SockAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // if the port is negative -> convert to 0 to let the windows sockets to choose port
    if (LocalSocketPort < 0)
        LocalSocketPort = 0;
    SockAddress.sin_port        = htons((WORD)LocalSocketPort);

    int Status = bind(m_Socket, (SOCKADDR*)&SockAddress, sizeof(SockAddress));

    return (Status == SOCKET_ERROR)? false :true;
}


//////////////////////////////////////////////////////////////////////////
// Normally, the sockets are not registered on the write events in order to prevent
// too many events coming(The on Send event is triggered whenever a socket is
// ready to send data). Since those events are needed in case the socket
// is connecting in order to know the connection status, or after a would block
// in order to know when is possible to resume sending, the socket is being
// registered to those events temporarily and being removed
// from them after it is no longer necessary.
void CBaseBerkeleySocket::SetRegistrationToWriteEvents(bool ShouldRegister)
{
    CSocketEventsManager* EventMgr = CSocketEventsManager::GetTheSocketEventsManager();
    CheckExpReturn(EventMgr != NULL, "CBaseBerkeleySocket::SetRegistrationToWriteEvents() - shouldn't happen");
    if (ShouldRegister)
    {
        if ( ! EventMgr->AddSocketToWriteEvents(m_Socket))
            LogEvent(LE_ERROR, "CBaseBerkeleySocket::SetRegistrationToWriteEvents - unable to set write events for socket [%x]", m_Socket);
    }
    else
    {
        if ( ! EventMgr->RemoveSocketFromWriteEvents(m_Socket))
            LogEvent(LE_WARNING, "CBaseBerkeleySocket::SetRegistrationToWriteEvents - unable to remove write events for socket [%x]", m_Socket);
    }
}


//////////////////////////////////////////////////////////////////////////
void CBaseBerkeleySocket::SetIsConnectingFlag(bool IsConnceting)
{
    m_IsConnecting = IsConnceting;
    SetRegistrationToWriteEvents(IsConnceting);
}


//////////////////////////////////////////////////////////////////////////
void CBaseBerkeleySocket::CheckingWouldBlockOnSend(int SendReturnStatus)
{
    if (SendReturnStatus != SOCKET_ERROR || m_IsLastSendWouldBlock)
        // everything OK or already in would block
        return;

    if (WSAGetLastError() == WSAEWOULDBLOCK)
    { // If there is a would block - start listening to write events to know when the would block will be over.
        m_IsLastSendWouldBlock = true;
        SetRegistrationToWriteEvents(true);
    }
}


//////////////////////////////////////////////////////////////////////////
// Since the function GetHostIPAsDWORD() is costly in performance, caching
// the last destination and using it when possible.
DWORD CBaseBerkeleySocket::CalculateAdressFromIp(const char* RemoteIp)
{
    // if similar to last destination , return the already calculated DWORD
    if (strcmp(RemoteIp, m_LastCalculatedIp.c_str()) == 0)
        return m_LastDwordAddress;
    
    // recalculate the last IP in DWORD
    m_LastDwordAddress = GetHostIPAsDWORD(RemoteIp);
    m_LastCalculatedIp = RemoteIp;
    return m_LastDwordAddress;
}

/*static*/ bool CBaseBerkeleySocket::CheckAddressValidity(const char* Ip)
{
    if (Ip == NULL)
        return false;
    if (strlen(Ip) == 0)
        return false;
    return (gethostbyname(Ip) != NULL);
}

//////////////////////////////////////////////////////////////////////////
int CBaseBerkeleySocket::GetLastError() const
{
    return WSAGetLastError();
}

const char * CBaseBerkeleySocket::GetLastErrorAsString() const
{
    return GetSocketErrorStr(GetLastError()).c_str();
}

//////////////////////////////////////////////////////////////////////////
//	ISocketEventsManagerEvents

//////////////////////////////////////////////////////////////////////////
void CBaseBerkeleySocket::OnSendEvent()
{
    CheckExpReturn(m_Sink != NULL, "CBaseBerkeleySocket::OnSend()");

    //If currently connecting trigger the OnConnected() event with true value.
    //If not connecting trigger the OnSend() event to notify the socket is ready to send.
    if (m_IsConnecting)
    {
        LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::OnSendEvent - socket [%x] is connected", m_Socket);
        SetIsConnectingFlag(false);
        m_Sink->OnConnect(BERKELEY_SOCKETS_OK);
    }
    else
    {
        m_Sink->OnSend();

        if (m_IsLastSendWouldBlock)
        { // In that case the socket is ready to send and the would block has been released.
          // It is time to remove the registration to the write events
            SetRegistrationToWriteEvents(false);
            m_IsLastSendWouldBlock = false;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
void CBaseBerkeleySocket::OnReceiveEvent()
{
    CheckExpReturn(m_Sink != NULL, "CBaseBerkeleySocket::OnReceive()");

    //If is in listening state, trigger the OnAccept() event. If not listening trigger
    //the OnReceive() event to notify that the data is ready to be taken.
    //Note that this event will also be received if the socket has been disconnected by remote side,
    //and in that case the OnConnectionClosed() event should have been triggered.
    //However, this case is being dealt with in the receive() function that should
    //be activate after the OnReceive() event was triggered
    if (m_IsListening)
    {
        LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::OnReceiveEvent - the listening socket [%x] has a pending connection", m_Socket);
        m_IsListening = false;
        m_Sink->OnAccept();
    }
    else
        m_Sink->OnReceive();
}


//////////////////////////////////////////////////////////////////////////
void CBaseBerkeleySocket::OnConnectFailed()
{
    CheckExpReturn(m_Sink != NULL, "CBaseBerkeleySocket::OnReceive()");

    //if currently connecting, trigger the OnConnected() event with false value
    if (m_IsConnecting)
    {
        LogEvent(LE_INFOLOW, "CBaseBerkeleySocket::OnConnectFailed - the socket [%x] connect request had failed", m_Socket);
        // figure out what is the error status
        int SocketStatus = BERKELEY_SOCKETS_OK;
        int ParamSize = sizeof(SocketStatus);
        if (getsockopt(m_Socket, SOL_SOCKET, SO_ERROR,
            (char*)&SocketStatus, &ParamSize) != BERKELEY_SOCKETS_OK)
            SocketStatus = SOCKET_ERROR;
        SetIsConnectingFlag(false);
        m_Sink->OnConnect(SocketStatus);
    }
    else
    {
        //Assert(false); // not expected
    }
}

