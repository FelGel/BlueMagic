// TcpSocket.cpp: implementation of the CTcpSocketClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TcpSocketClient.h"
#include "BerkeleySocketsUtils.h"
#include "Common/LogEvent.h"
#include "Common/CriticalSection.h"
#include "Common/Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////
// class CTcpSocketClient

CTcpSocketClient::CTcpSocketClient():
	m_IsCreated(false),
	m_IsConnected(false),
	m_LastConnectionTryTime(0),
	m_ReconnectItervalInSec(DefaultReconnectionTime),
    m_Sink(NULL),
    m_AutoReconnect(false) ,
    m_SinkTimerRate(0),  
    m_ReconnectTimerRate(0),
    m_LastSinkTimeout(0),
    m_ConnectRetries(0)
{
    LogEvent(LE_INFOLOW, "CTcpSocketClient() CTor");
}

CTcpSocketClient::~CTcpSocketClient()
{
	//CTimerManager& RefTimer = CTimerManager::GetTheTimerManager();
	m_Socket.StopTimer();
	//RefTimer.KillTimer(m_timerID);
}

void CTcpSocketClient::Init(const char* ContextName, ITcpSocketClientEvents* Sink,
                            ITcpIncomingFilter* IncomingFilter,
                            ITcpOutgoingFilter* OutgoingFilter,
                            CBaseKeepAliveHandler* KeepAliveHandler)
{
    m_ContextName = ContextName;
    m_Sink = Sink;
    m_Socket.SetFilters(OutgoingFilter == NULL ? NULL : OutgoingFilter->CloneOutgoingFilter(),
        IncomingFilter == NULL ? NULL : IncomingFilter->CloneIncomingFilter());
    m_Socket.SetKeepAliveHandler(KeepAliveHandler == NULL ? NULL : KeepAliveHandler->Clone());
}

bool CTcpSocketClient::Create(const char* ServerIpAddress, int ServerPort, 
                              const char* LocalIp, int LocalPort,
                              int ReconnectItervalInSec, bool UseAutoReconnect)
{
// 	LogEvent(LE_DEBUG, "TcpSocketClient::Create - started. Server IP %s, Server port %d, Reconnect interval %d seconds.",
// 		ServerIpAddress, ServerPort, ReconnectItervalInSec);
    if (!CBaseBerkeleySocket::CheckAddressValidity(ServerIpAddress))
    {
        LogEvent(LE_ERROR, "CTcpSocketClient(%s)::Create: ServerIpAddress %s is not valid", m_ContextName.c_str(), ServerIpAddress);
        return false;
    }

    if(LocalIp == NULL)
        m_LocalIpAddress = "0.0.0.0";
    else
        m_LocalIpAddress = LocalIp;
    m_LocalPort = LocalPort;


    if (m_Sink == NULL)
    {
        LogEvent(LE_ERROR, "CTcpSocketClient(%s)::Create: m_Sink == NULL", m_ContextName.c_str());
        return false;
    }

    m_ServerPort = ServerPort;
    m_ServerIpAddress = ServerIpAddress == NULL ? "" : ServerIpAddress;

	bool Success = RecreateSocket(true); //create the socket and try to connect
    if(!Success)
        LogEvent(LE_ERROR, "CTcpSocketClient(%s)::Create(), RecreateSocket failed", m_ContextName.c_str());

    //initialize reconnect mechanism
    if(ReconnectItervalInSec > 0)
		m_ReconnectItervalInSec = ReconnectItervalInSec;
    const UINT ReconnectTimeMilli = 100;
    m_ReconnectTimerRate = ReconnectTimeMilli;
    if(m_Socket.GetCurrentTimerRate() > m_ReconnectTimerRate)
        m_Socket.StartTimer(m_ReconnectTimerRate, m_ContextName.c_str());
    m_LastConnectionTryTime = time(NULL);
    m_AutoReconnect = UseAutoReconnect; 

    m_IsCreated = true;
	return Success;
}

bool CTcpSocketClient::CloseAndCreateSocket(bool UseAutoReconnect, bool ReconnectNow)
{
// 	LogEvent(LE_DEBUG, "TcpSocketClient::CloseAndCreateSocket - started. Reconnect = %s.",
// 		ReconnectNow ? "TRUE" : "FALSE");

    m_AutoReconnect = UseAutoReconnect; 
    return RecreateSocket(ReconnectNow);
}

bool CTcpSocketClient::RecreateSocket(bool Reconnect)
{
    //prevent connecting twice from the creating thread and the CommThread
    //    CCriticalSectionLocker Locker(m_ConnectLock);

    if(m_IsConnected)
    {
//         LogEvent(LE_DEBUG, "TcpSocketClient::RecreateSocket - socket already connected.");
        return true;
    }

    LogEvent(LE_INFO, "TcpSocketClient(%s)::RecreateSocket.", m_ContextName.c_str());
    m_Socket.Close();
    if (!m_Socket.Create(m_LocalPort, m_LocalIpAddress.c_str(), this))
    {
        LogEvent(LE_ERROR, 
            "CTcpSocketClient(%s)::RecreateSocket: Error in Create(%d), Error %s\n",
            m_ContextName.c_str(), m_ServerPort, GetSocketErrorStr(GetLastError()).c_str());
        return false;
    }

    //??
    if (m_Socket.SetSockOptNoDelay(true) == false)
    {
        LogEvent(LE_ERROR, 
            "TcpSocketClient(%s)::RecreateSocket: Error in setting no delay, Error %s",
            m_ContextName.c_str(), GetSocketErrorStr(WSAGetLastError()).c_str());
        return false;
    }
    //??

    if(Reconnect)
        Connect();

    return true;
}

bool CTcpSocketClient::IsConnected() const
{
	return m_IsConnected;
}

void CTcpSocketClient::SetAutoReconnection(bool AutoReconnect)
{
    m_AutoReconnect = AutoReconnect;
}

void CTcpSocketClient::MaintainConnectionStatus(bool Connected, bool ReconnectNow)
{
// 	LogEvent(LE_DEBUG, "TcpSocketClient::MaintainConnectionStatus : method param Connected = %s.", Connected ? "TRUE" : "FALSE");
    if (Connected != m_IsConnected)
    {
        m_IsConnected = Connected;
        // Notify sink
        if (m_Sink != NULL)
        {
            if (m_IsConnected)
                m_Sink->OnConnectedToServer();
            else
                m_Sink->OnDisconnectedFromServer();
        }
        else
            LogEvent(LE_ERROR, "CTcpSocketClient(%s)::MaintainConnectionStatus: m_Sink=NULL", m_ContextName.c_str());

        
        if (!m_IsConnected && m_AutoReconnect)
            // Recreate the socket try to connect according to the input parameters
            RecreateSocket(ReconnectNow);
    }

    //Enable the keep alive according to the connection status
    m_Socket.EnableKeepAlive(m_IsConnected);
}

void CTcpSocketClient::Connect()
{
    LogEvent(LE_INFOLOW, "CTcpSocketClient(%s)::Connect Try connect", m_ContextName.c_str());

    //ensure that the socket is created
    //Assert(!m_Socket.IsClosed());
    const int RECREATE_SOCKET_CONNECT_RETRIES = 3;
    if(!m_Socket.IsClosed() && m_ConnectRetries >= RECREATE_SOCKET_CONNECT_RETRIES)
    {
        m_ConnectRetries = 0;
        if(!RecreateSocket(false)) //only create the socket, without connect
        {
            return;
        }
    }
    bool Result = m_Socket.Connect(m_ServerIpAddress.c_str(), m_ServerPort);
    if (!Result)
    {
        m_ConnectRetries++;
        int Error = GetLastError();
        // WSAEISCONN means that the socket is already connected.
        if (Error == WSAEISCONN)
            MaintainConnectionStatus(true, true);
        else
        {
            if (Error != WSAEWOULDBLOCK)
                LogEvent(Error == WSAECONNREFUSED ? LE_INFO : LE_WARNING, 
				    "CTcpSocketClient(%s)::Connect: error %s", 
                    m_ContextName.c_str(), GetWin32Error(Error).c_str()/*GetConnectErrorStr(Error).c_str()*/);
//        else
//            LogEvent(LE_INFO , "CTcpSocketClient(%s)::Connect: Socket is blocked", 
//                m_ContextName.c_str());
        }
    }
    else
        m_ConnectRetries = 0;

    m_LastConnectionTryTime = time(NULL);
}

bool CTcpSocketClient::SendMessage(const void* Data, int DataSize, bool IsPriorityMsg,
								   ELogSeverity WouldBlockLogLevel, int NumWouldBlockResends)
{
	return m_Socket.SendMessage(Data, DataSize, IsPriorityMsg, WouldBlockLogLevel, NumWouldBlockResends);
}

void CTcpSocketClient::CloseConnection()
{
    SetAutoReconnection(false);
	LogEvent(LE_INFO, "TcpSocketClient(%s)::CloseConnection ", m_ContextName.c_str());
	m_Socket.Close();
	MaintainConnectionStatus(false, false);
}

/*virtual */
void CTcpSocketClient::OnConnect(int nErrorCode)
{
    //LogEvent(LE_WARNING, "CTcpSocketClient::OnConnect(%d)", nErrorCode);
	if(nErrorCode == 0)
	{
		//LogEvent(LE_WARNING, "CTcpSocketClient::OnConnect(): Connected\n");
        MaintainConnectionStatus(true, false);
	}
	else 
		LogEvent(nErrorCode == WSAECONNREFUSED ? LE_INFO : LE_WARNING,
			"CTcpSocketClient(%s)::OnConnect() - connect error [%s]", m_ContextName.c_str(), GetSocketErrorStr(nErrorCode).c_str());
}

void CTcpSocketClient::OnSend()
{
	if(m_Sink)
		m_Sink->OnReadyForSend();
	else
		LogEvent(LE_ERROR, "TcpSocketClient(%s)::OnSend : no sink.", m_ContextName.c_str());
}

/*virtual*/ 
void CTcpSocketClient::OnClose()
{
	LogEvent(LE_INFO, "CTcpSocketClient(%s)::OnClose()", 
        m_ContextName.c_str());
    
	MaintainConnectionStatus(false, false);
}

/*virtual*/ 
void CTcpSocketClient::OnMessage(const void* Data, int DataSize)
{
	if(m_Sink)
		m_Sink->OnMessageFromServer(Data, DataSize);
	else
		LogEvent(LE_ERROR, "CTcpSocketClient(%s)::OnMessage: m_Sink=NULL", m_ContextName.c_str());
}

bool CTcpSocketClient::StartTimer(UINT TimerRate)
{ 
    if(TimerRate <= 0)
        return false;

    m_SinkTimerRate = TimerRate;   
    m_LastSinkTimeout = GetTickCount();
    if(m_Socket.GetCurrentTimerRate() <= 0 || m_SinkTimerRate < m_Socket.GetCurrentTimerRate())
        return m_Socket.StartTimer(m_SinkTimerRate, m_ContextName.c_str());

    return true; 
}
bool CTcpSocketClient::StopTimer()
{
    //stop the sending timer events to the Sink
    m_SinkTimerRate = 0;
    if(m_Socket.GetCurrentTimerRate() != m_ReconnectTimerRate)
        return m_Socket.StartTimer(m_ReconnectTimerRate, m_ContextName.c_str());
    return true;
}

/*virtual*/ 
void CTcpSocketClient::OnSocketTimer()
{
    //prevent connecting twice from the creating thread and the CommThread
//    CCriticalSectionLocker Locker(m_ConnectLock);

    if (!m_IsCreated || !m_AutoReconnect)
        return;

//    LogEvent(LE_INFO, "CTcpSocketClient::OnSocketTimer\n");

    if (!m_IsConnected)
    {
        time_t CurrentTime = time(NULL);
        if (CurrentTime - m_LastConnectionTryTime > m_ReconnectItervalInSec )
            Connect();
    }

    if(m_Sink && m_SinkTimerRate > 0)
    {
        if(GetTickCount() - m_LastSinkTimeout > m_SinkTimerRate)
        {  
            m_LastSinkTimeout = GetTickCount();
            m_Sink->OnClientSocketTimeout();
        }
    }
}

bool CTcpSocketClient::GetPeerData(std::string& Ip, UINT& Port)
{
    return m_Socket.GetPeerData(Ip, Port);
}

bool CTcpSocketClient::GetSockName(std::string& Ip, UINT& Port)
{
    return m_Socket.GetSockName(Ip, Port);
}
///////////////////////////////////////////////////////////////
// Test
///////////////////////////////////////////////////////////////

#ifdef _TEST

#include "Common\TimerManager.h"
#include "TcpSocketTest.h"

static class CSocketClientTest* TheSocketClientTester = NULL;

class CSocketClientTest : 
	public ITcpSocketClientEvents
{
public:
    CSocketClientTest() :
        m_IsConnected(false),
        m_SendCounter(0),
        m_ReceiveCounter(0)
    {
    }

	~CSocketClientTest()
	{
		m_Socket.StopTimer();
	}

    enum { TIMER_RESOLUTION = 1000 }; // 10-1000
    bool Create()
    {
//        const char* ConfigSection = "TcpSocket";
//        m_Sender = false;

        std::string ServerIpAddress = "127.0.0.1";
        //std::string ServerIpAddress = GetConfigString(ConfigSection, "ServerIpAdderss", "127.0.0.1\n");

        int ServerPort = 2048;
        //int ServerPort = GetConfigInt(ConfigSection, "ServerPort", 2048);


        m_IsConnected = false;

        LogEvent(LE_INFO, "CSocketClientTest: Ip Address %s, Port %d",
            ServerIpAddress.c_str(), ServerPort); 

		m_Socket.StartTimer(TIMER_RESOLUTION);

        m_Socket.Init("Client Socket Test", this, &m_Parser, NULL);
        return m_Socket.Create(ServerIpAddress.c_str(), ServerPort);
    }

    bool Send(const void* Data, int Size)
    {
//        LogEvent(LE_DEBUG, "CSocketClientTest: Sending size %d", Size);
        return m_Socket.SendMessage(Data, Size);
    }

    bool Send(int Start, int Size)
    {
        TestMessage Message;

        int TotalSize = Message.Fill(Start, Size);
        return Send(&Message, TotalSize);
    }

private:
	//ITcpSocketClientEvents
    virtual void OnMessageFromServer(const void* Data, int DataSize)
    {
//        const int CountForDisplay = 1000 / TIMER_RESOLUTION;
//        if (m_ReceiveCounter % CountForDisplay == 0)
//         LogEvent(LE_DEBUG, "CSocketClientTest::OnMessageFromServer [%d] (%d)", m_ReceiveCounter, DataSize);

        const TestMessage* Message = reinterpret_cast<const TestMessage*>(Data);

        Assert(Message->Check(m_ReceiveCounter, DataSize));
        m_ReceiveCounter++;

//        // Rebound message only if not Sender - otherwise will get endless resend
//        if (MessageOK) 
//            m_Socket.SendMessage(Data, DataSize);
    }
    
	virtual void OnConnectedToServer()
    {
        LogEvent(LE_INFOLOW, "CSocketClientTest::OnConnected\n"); 
        m_IsConnected = true;
    }

    virtual void OnDisconnectedFromServer()
    {
        LogEvent(LE_INFOLOW, "CSocketClientTest::OnDisconnected\n"); 
        m_IsConnected = false;
    }

    virtual void OnClientSocketTimeout()
    {
        if (m_IsConnected)
        {
            int Size = Random(4000) + 10;
            Send(m_SendCounter++, Size);
        }
    }

    CTcpSocketClient   m_Socket;
    bool               m_IsConnected;
//    bool               m_Sender;
    int                m_SendCounter;
    int                m_ReceiveCounter;
    CTestMessageParser m_Parser;

	//CTimerManager m_TimerManager;
	int m_timerID;
};

///////////////////////////////////////////////////////////////////////////////////

void TestTcpSocketClient()
{
    if (TheSocketClientTester == NULL)
    {
        LogEvent(LE_INFOHIGH, "TestTcpSocketClient: Socket Created\n"); 
        TheSocketClientTester = new CSocketClientTest;
        TheSocketClientTester->Create();
    }
    else
    {
        delete TheSocketClientTester;
        TheSocketClientTester = NULL;
        LogEvent(LE_INFOHIGH, "TestTcpSocketClient: Socket Deleted\n"); 
    }
}

#endif // _TEST
