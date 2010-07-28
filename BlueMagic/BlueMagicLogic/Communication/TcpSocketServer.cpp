// TcpSocketServer.cpp: implementation of the CTcpSocketServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Common/LogEvent.h"
#include "Common/Utils.h"
#include "TcpSocketServer.h"
#include "BerkeleySocketsUtils.h"
#include "Common/CriticalSection.h"
#include "Common/CollectionHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTcpSocketServer::CTcpSocketServer() : 
	m_MultiClient(false),
	m_Sink(NULL),
	m_OutgoingFilter(NULL),
    m_IncomingFilter(NULL),
	m_LastConnectionId(0),
    m_Closing(false),
    m_KeepAliveHandler(NULL)
{
}

CTcpSocketServer::~CTcpSocketServer()
{
    Close();
}

void CTcpSocketServer::Init(ITcpSocketServerEvents *Sink, 
                            ITcpIncomingFilter* IncomingFilter,
                            ITcpOutgoingFilter* OutgoingFilter,
                            CBaseKeepAliveHandler* KeepAliveHandler)
{
    m_Closing = false;
	m_Sink = Sink;

    RemoveFilters();
    if (IncomingFilter != NULL)
	    m_IncomingFilter = IncomingFilter->CloneIncomingFilter();
    if (OutgoingFilter != NULL)
        m_OutgoingFilter = OutgoingFilter->CloneOutgoingFilter();
    if (KeepAliveHandler != NULL)
        m_KeepAliveHandler = KeepAliveHandler->Clone();

	//LogEvent(LE_DEBUG, "CTcpSocketServer::Advise: Max Message Size: %d", m_Parser->MaxMessageSize());
    //m_Socket.SetKeepAliveHandler(CloneKeepAliveHandler());
    //m_Socket.SetFilters(CloneOutgoingFilter(), CloneIncomingFilter());
}

bool CTcpSocketServer::Create(int ServerPort, const char* ServerIP, bool MultiClient)
{
    if (m_Sink == NULL)
    {
        LogEvent(LE_ERROR, "CTcpSocketServer::Create: m_Sink == NULL\n");
        return false;
    }
	m_MultiClient = MultiClient;
    m_ServerPort = ServerPort;
    m_ServerIP = (ServerIP==NULL) ? "" : ServerIP;
    m_IsCreated = true;
    return CloseAndCreateSocket();
}

bool CTcpSocketServer::CloseAndCreateSocket()
{
    m_Socket.Close();
    if (!m_Socket.Create(m_ServerPort, m_ServerIP.c_str(), this))
    {
        LogEvent(LE_ERROR, 
			"TcpSocketServer::CloseAndCreateSocket: Error in Create(%s:%d), Error %s",
            m_ServerIP.c_str(), m_ServerPort, GetSocketErrorStr(WSAGetLastError()).c_str());
        return false;
    }

//??
    //TCP_NODELAY which disables the Nagle algorithm for send coalescing
    if (m_Socket.SetSockOptNoDelay(true) == false)
    {
        LogEvent(LE_ERROR, 
            "TcpSocketServer::CloseAndCreateSocket: Error in setting no delay(%s:%d), Error %s",
            m_ServerIP.c_str(), m_ServerPort, GetSocketErrorStr(WSAGetLastError()).c_str());
        return false;
    }
//??

	if (!m_Socket.Listen())
	{
        LogEvent(LE_ERROR, 
			"CTcpSocketServer::CloseAndCreateSocket: Error in Listen(), Error %s",
			GetSocketErrorStr(WSAGetLastError()).c_str());
		return false;
	}

	enum { TIMEOUT_INTERVAL = 5000};
	m_Socket.StartTimer(TIMEOUT_INTERVAL, "CTcpSocketServer");
    return true;
}

void CTcpSocketServer::Close()
{
    if(m_Closing)
    {
        Assert(m_Socket.IsClosed());
        return;
    }

    m_Socket.Close();

    // Coping the connection map to a temp copy and after clearing the original map
    // can release the lock and close all the sockets without fearing deadlock with 
    // the socket event lock.
    ConnectionsMap CopyOfConnectionMap;
    CloseDataStructures(CopyOfConnectionMap);

    ConnectionsIter Iter = CopyOfConnectionMap.begin();
    ConnectionsIter EndIter = CopyOfConnectionMap.end();
    for(; Iter != EndIter; ++Iter )
    {
        //Todo: decide if need to close connections explicitly
        CTcpSocketData* SocketData = Iter->second;
        SocketData->CloseDataConnection();
        delete SocketData;
    }

    m_Closing = true;

}

// Here closing all the data structures that should be closed inside the lock
void CTcpSocketServer::CloseDataStructures(ConnectionsMap& CopyOfConnectionMap)
{
    CCriticalSectionLocker Locker(m_MapGuard);
    // coping the connection to a temp map, since if closing and deleting under the 
    // lock it might lead to a deadlock with the Socket events lock
    CopyOfConnectionMap = m_ConnectionsMap;

    m_ConnectionsMap.clear();

    //Deleting all the closed sockets from the vector
    int Count = m_ClosedSockets.size();
    for (int i = 0 ; i < Count ; i++)
        delete m_ClosedSockets[i];
    m_ClosedSockets.clear();

    RemoveFilters();
}


int CTcpSocketServer::GetNextConnectionId()
{
    Assert(m_LastConnectionId >= 0);
    if (m_LastConnectionId < 0  ||  m_LastConnectionId == INT_MAX)
        m_LastConnectionId = 0;
	m_LastConnectionId++;
	return m_LastConnectionId;
}

bool CTcpSocketServer::SendMessage(const void* Data, int DataSize, int ConnectionId,
                                   bool IsPriorityMsg, ELogSeverity WouldBlockLogLevel , int NumWouldBlockResends)
{
    CCriticalSectionLocker Locker(m_MapGuard);
	if(ConnectionId != SEND_ALL)
	{
		CTcpSocketData* SocketData;
		if(!GetValueFromMap(m_ConnectionsMap, ConnectionId, SocketData))
		{
//			Assert(false);
			LogEvent(LE_WARNING, "CTcpSocketServer::SendMessage: ConnectionId (%d) not found.", ConnectionId);
			return false;
		}
		return SocketData->SendMessage(Data, DataSize, IsPriorityMsg, WouldBlockLogLevel, NumWouldBlockResends);
	}
	else
	{
		ConnectionsIter Iter;
		ConnectionsIter BeginIter = m_ConnectionsMap.begin();
		ConnectionsIter EndIter = m_ConnectionsMap.end();

		for(Iter=BeginIter ; Iter!=EndIter ; ++Iter)
		{
            CTcpSocketData* SocketData = Iter->second;
			if(!SocketData->SendMessage(Data, DataSize, IsPriorityMsg, WouldBlockLogLevel, NumWouldBlockResends))
				return false;
		}
	}
	return true;
}

bool CTcpSocketServer::CloseClientConnection(int ConnectionId)
{
    LogEvent(LE_INFO, "TcpSocketServer::CloseClientConnection : connection id = %d.", ConnectionId);
    return RemoveClientConnection(ConnectionId, true);
}

ITcpOutgoingFilter* CTcpSocketServer::CloneOutgoingFilter()
{
    if(m_OutgoingFilter == NULL)
        return NULL;
    else
        return m_OutgoingFilter->CloneOutgoingFilter();
}

ITcpIncomingFilter* CTcpSocketServer::CloneIncomingFilter()
{
    if(m_IncomingFilter == NULL)
        return NULL;
    else
        return m_IncomingFilter->CloneIncomingFilter();
}

CBaseKeepAliveHandler* CTcpSocketServer::CloneKeepAliveHandler()
{
    if(m_KeepAliveHandler == NULL)
        return NULL;
    else
        return m_KeepAliveHandler->Clone();
}

void CTcpSocketServer::RemoveFilters()
{
    //remove filters and keep alive handler
    if(m_KeepAliveHandler != NULL)
    {
        m_KeepAliveHandler->Advise(NULL);
        delete m_KeepAliveHandler;
        m_KeepAliveHandler = NULL;
    }

    if(m_IncomingFilter != NULL)
    {
        m_IncomingFilter->AdviseIncomingFilterEvents(NULL);
        delete m_IncomingFilter;
        m_IncomingFilter = NULL;
    }

    if(m_OutgoingFilter != NULL)
    {
        m_OutgoingFilter->AdviseOutgoingFilterEvents(NULL);
        delete m_OutgoingFilter;
        m_OutgoingFilter = NULL;
    }
}


bool CTcpSocketServer::RemoveClientConnection(int ConnectionId, bool DeleteSocket)
{
    CTcpSocketData* SocketData = ExtractConnectionFromMap(ConnectionId);
    if (SocketData == NULL)
    {
        LogEvent(LE_ERROR, "CTcpSocketServer::RemoveClientConnection: ConnectionId (%d) not found.", ConnectionId);
        return false;
    }

    if(DeleteSocket)
    {
        //close the socket and move it to the closed sockets vector
        SocketData->CloseDataConnection(); // No need to lock since the object was removed from the map
        CCriticalSectionLocker Locker(m_MapGuard); // protect adding to the ClosedSockets map
        m_ClosedSockets.push_back(SocketData);
    }

    return true;
}

void CTcpSocketServer::OnSocketTimer()
{
    CCriticalSectionLocker Locker(m_MapGuard); // protect deleting from the ClosedSockets map
    for(unsigned int i = 0; i < m_ClosedSockets.size(); i++)
		delete m_ClosedSockets[i];

	m_ClosedSockets.clear();
	
}


// returns a pointer to a data socket. Stops listening to its
// events. Leaves the connection open.
CTcpSocketData* CTcpSocketServer::DetachSocket(int ConnectionId)
{
	CCriticalSectionLocker Locker(m_MapGuard);

	ConnectionsIter Iter = m_ConnectionsMap.find(ConnectionId);
	if(Iter == m_ConnectionsMap.end())
	{
		LogEvent(LE_ERROR, "TcpSocketServer::DetachSocket: Can't find Connection (%d)",	
			ConnectionId);
		return NULL;
	}		 
	CTcpSocketData* SocketData = Iter->second;
	SocketData->Advise(NULL); // detach from this socket events
	m_ConnectionsMap.erase(Iter);
	return SocketData;
}

bool CTcpSocketServer::GetPeerName(int ConnectionId, std::string& PeerIP, 
								   unsigned int& PeerPort)
{
    CCriticalSectionLocker Locker(m_MapGuard);

    CTcpSocketData* SocketData;
    if(!GetValueFromMap(m_ConnectionsMap, ConnectionId, SocketData))
	{
		LogEvent(LE_ERROR, "TcpSocketServer::GetPeerName: Cant find Connection (%d)",	
			ConnectionId);
		return false;
	}		 
	return SocketData->GetPeerData(PeerIP, PeerPort);
}

bool CTcpSocketServer::GetServerAdrs(std::string& IP,
									 unsigned int& Port)
{
	if(m_IsCreated)
		return m_Socket.GetSockName(IP, Port);
	else
		return false;
}

bool CTcpSocketServer::InsertConnectionToMap(int ConnectionId, CTcpSocketData* SocketData)
{
    CCriticalSectionLocker Locker(m_MapGuard);
    return InsertValueToMap(m_ConnectionsMap, ConnectionId, SocketData);
}

CTcpSocketData* CTcpSocketServer::ExtractConnectionFromMap(int ConnectionId)
{
    CCriticalSectionLocker Locker(m_MapGuard);
    ConnectionsIter Iter = m_ConnectionsMap.find(ConnectionId);
    if (Iter == m_ConnectionsMap.end())
        return NULL;

    CTcpSocketData* Result = Iter->second;
    m_ConnectionsMap.erase(Iter);
    return Result;
}


/////////////////////////////////////////////////
// ITcpSocketSerivceEvents
/////////////////////////////////////////////////

/*virtual*/ 
void CTcpSocketServer::OnAccept()
{
    LogEvent(LE_INFOLOW, "CTcpSocketServer::OnAccept()");
	Accept();
}

/*virtual*/
int CTcpSocketServer::Accept()
{
    SOCKET ConnectedSocket = m_Socket.Accept(m_MultiClient);
    if (ConnectedSocket == INVALID_SOCKET)
    {
        LogEvent(LE_ERROR, "CTcpSocketServer::Accept error %d", GetLastError());
        return -1;
    }

    int Handle = GetNextConnectionId();
	CTcpSocketData* DataSocket = new CTcpSocketData;

    DataSocket->Init(Handle, this, ConnectedSocket, 
        CloneIncomingFilter(), CloneOutgoingFilter(), CloneKeepAliveHandler());

    if(!InsertConnectionToMap(Handle, DataSocket))
    {
        LogEvent(LE_ERROR, "CTcpSocketServer::Accept: error in insert connection %d to map", 
            Handle);
        delete DataSocket;
		return -1;
	}

	if(m_Sink)
		m_Sink->OnConnectedToClient(Handle);
	else
		LogEvent(LE_ERROR, "CTcpSocketServer::Accept: m_Sink=NULL\n");
	
	// Set the connected socket hSocket to out hSocket
    if(!m_MultiClient)
		m_Socket.Close();

	return Handle;
}

/*virtual*/ 
void CTcpSocketServer::OnClose()
{
	CloseAndCreateSocket();
}


/////////////////////////////////////////////////
// ITcpSocketDataEvents
/////////////////////////////////////////////////
/*virtual*/ 
void CTcpSocketServer::OnSocketDataMessage(const void* Data, 
										   int DataSize, 
										   int ConnectionId)
{
	Assert(m_Sink);
	if(!m_Sink)
	{
		LogEvent(LE_ERROR, "CTcpSocketServer::OnSocketDataMessage: m_sink=0\n");
		return;
	}
	m_Sink->OnReceivedMessageFromClient(Data, DataSize, ConnectionId);
}

		
/*virtual*/ 
void CTcpSocketServer::OnSocketDataDisconnected(int ConnectionId)
{
    LogEvent(LE_INFO, "TcpSocketServer::OnSocketDataDisconnected : connection id = %d.", ConnectionId);

	if(m_Sink)
		m_Sink->OnDisconnectedFromClient(ConnectionId);
	else
		LogEvent(LE_ERROR, "CTcpSocketServer::OnSocketDataDisconnected: m_Sink=NULL\n");

    //delete the socket data
    if(!RemoveClientConnection(ConnectionId, true))
	{
		LogEvent(LE_ERROR, 
			"CTcpSocketServer::OnSocketDataDisconnected(): Failed to close Connection %d",
			ConnectionId);
	}

	if(!m_MultiClient)
	{
		CloseAndCreateSocket();
	}
}


///////////////////////////////////////////////////////////////
// Test
///////////////////////////////////////////////////////////////

#ifdef _TEST

#include "Common\TimerManager.h"
#include "TcpSocketTest.h"

static class CSocketServerTest* TheSocketServerTester = NULL;

class CSocketServerTest : 
	public ITcpSocketServerEvents
{
public:
    CSocketServerTest() //:
    {
		m_ClientMap.clear();
    }

	~CSocketServerTest()
	{
	}

    enum { TIMER_RESOLUTION = 10 }; // 10-1000
    bool Create(bool MultiClient)
    {
        int ServerPort = 2048;
		std::string ServerIP = "127.0.0.1";
        LogEvent(LE_INFOLOW, "TestTcpSocketServer: Port %d", ServerPort); 
        m_Socket.Init(this, &m_Parser);
        return m_Socket.Create(ServerPort, ServerIP.c_str(), MultiClient);
    }

    bool Send(const void* Data, int Size, int ConnectionId)
	{
        return m_Socket.SendMessage(Data, Size, ConnectionId);
    }

    bool Send(int Start, int Size, int ConnectionId)
    {
        TestMessage Message;

        int TotalSize = Message.Fill(Start, Size);
        return Send(&Message, TotalSize, ConnectionId);
    }

private:
	//ITcpSocketServerEvents
    virtual void OnReceivedMessageFromClient(const void* Data, int DataSize, int ConnectionId)
    {
		ClientIter Iter = m_ClientMap.find(ConnectionId);
		if(Iter == m_ClientMap.end())
		{
			LogEvent(LE_ERROR, "TestTcpSocketServer::OnReceivedMessageFromClient: Didn't connect to this Connection (%d) ???", ConnectionId);
		}		 

//         LogEvent(LE_DEBUG, "TestTcpSocketServer::OnReceivedMessageFromClient[%d] [%d] (%d)", ConnectionId, Iter->second, DataSize);

        const TestMessage* Message = reinterpret_cast<const TestMessage*>(Data);

        bool MessageOK = Message->Check(Iter->second, DataSize);
        Iter->second++;
        
        // Rebound message only if not Sender - otherwise will get endless resend
        if (MessageOK) 
            m_Socket.SendMessage(Data, DataSize, ConnectionId);
    }

	virtual void OnConnectedToClient(int ConnectionId)
    {
        LogEvent(LE_INFOLOW, "CSocketServerTest::OnConnectedToClient: Connection %d", ConnectionId); 

		ClientMap::value_type Value(ConnectionId, 0);
		std::pair<ClientIter, bool> InsResult = m_ClientMap.insert(Value);
		Assert(InsResult.second);

		if (!InsResult.second)
		{
			LogEvent(LE_ERROR, "CSocketServerTest::OnConnectedToClient: error in insert %d", ConnectionId);
			return;
		}

		LogEvent(LE_INFOLOW,"Number of clients %d", m_ClientMap.size());
    }

    virtual void OnDisconnectedFromClient(int ConnectionId)
    {
        LogEvent(LE_INFOLOW, "CSocketServerTest::OnDisconnectedFromClient: Connection %d", ConnectionId); 

		int RemoveItems = m_ClientMap.erase(ConnectionId);
		Assert(RemoveItems==1);

		if(RemoveItems !=1 )
		{
			LogEvent(LE_ERROR, "CSocketServerTest::OnDisconnectedFromClient RemoveItems=%d. Should be=1\n");
		}

		LogEvent(LE_INFOLOW,"Number of clients %d", m_ClientMap.size());

    }

    CTcpSocketServer	m_Socket;
    CTestMessageParser	m_Parser;
	typedef std::map<int /*Cliend Handle*/, int /*ReceiveCounter*/> ClientMap;
	typedef ClientMap::iterator ClientIter;
	ClientMap m_ClientMap;
};

///////////////////////////////////////////////////////////////////////////////////

void TestTcpSocketServer(bool MultiClient)
{
    if (TheSocketServerTester == NULL)
    {
        LogEvent(LE_INFOHIGH, "TestTcpSocketServer: Socket Created\n"); 
        TheSocketServerTester = new CSocketServerTest;
        TheSocketServerTester->Create(MultiClient);
    }
    else
    {
        delete TheSocketServerTester;
        TheSocketServerTester = NULL;
        LogEvent(LE_INFOHIGH, "TestTcpSocketServer: Socket Deleted\n"); 
    }
}

#endif // _TEST
