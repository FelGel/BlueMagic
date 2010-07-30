// TcpSocketMultiServer.h: interface for the CTcpSocketServer class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "TcpSocketService.h"
#include "TcpSocketData.h"
#include <map>


class ITcpSocketServerEvents
{
public:
	virtual void OnConnectedToClient(int ConnectionId) = 0;
	virtual void OnDisconnectedFromClient(int ConnectionId) = 0;
	virtual void OnReceivedMessageFromClient(const void* Data, int DataSize, int ConnectionId) = 0;
};


class CTcpSocketServer :
	public ITcpSocketServiceEvents, 
	public ITcpSocketDataEvents
{
public:
	CTcpSocketServer();
	virtual ~CTcpSocketServer();

    void Init(ITcpSocketServerEvents *Sink, ITcpIncomingFilter* IncomingFilter = NULL,
                ITcpOutgoingFilter* OutgoingFilter = NULL,
                CBaseKeepAliveHandler* KeepAliveHandler = NULL);

    bool Create(int ServerPort, const char* ServerIP = NULL, bool MultiClient = false);

	enum { SEND_ALL = -1};
	bool SendMessage(const void* Data, int DataSize, 
        int ConnectionId=SEND_ALL,  bool IsPriorityMsg = true, 
        ELogSeverity WouldBlockLogLevel = LE_ERROR, int NumWouldBlockResends = 0);

	bool CloseClientConnection(int ConnectionId);
	void Close();
	CTcpSocketData* DetachSocket(int ConnectionId);

	bool GetPeerName(int ConnectionId, std::string& PeerIP, unsigned int& PeerPort);
	bool GetServerAdrs(std::string& IP, unsigned int& Port);

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
    // Hide copy ctor and assignment operator
    CTcpSocketServer(const CTcpSocketServer &);
    CTcpSocketServer & operator=(const CTcpSocketServer &);

protected:
	virtual int Accept();

private:
	// ITcpSocketSerivceEvents
	virtual void OnAccept();
	virtual void OnClose();
    virtual void OnConnect(int /* nErrorCode */){}
    virtual void OnMessage(const void* /* Data */, int /* DataSize */) {}
	virtual void OnSocketTimer();


	// ITcpSocketDataEvents
    virtual void OnSocketDataMessage(const void* Data, 
		int DataSize, int ConnectionId);
    virtual void OnSocketDataDisconnected(int ConnectionId);


	bool CloseAndCreateSocket();
	int GetNextConnectionId();
    bool RemoveClientConnection(int ConnectionId, bool DeleteSocket);
    ITcpOutgoingFilter* CloneOutgoingFilter();
    ITcpIncomingFilter* CloneIncomingFilter();
    CBaseKeepAliveHandler* CloneKeepAliveHandler();
    void RemoveFilters(); //remove filters and keep alive handler


private:
    typedef std::map<int /*ConnectionId*/, CTcpSocketData*> ConnectionsMap;
    typedef ConnectionsMap::iterator ConnectionsIter;
    void CloseDataStructures(ConnectionsMap& CopyOfConnectionMap);

    // Map functions
    bool InsertConnectionToMap(int ConnectionId, CTcpSocketData* SocketData);
    CTcpSocketData* ExtractConnectionFromMap(int ConnectionId); // find & remove

private:
	CTcpSocketService		m_Socket;
	ITcpSocketServerEvents*	m_Sink;
    ITcpIncomingFilter*     m_IncomingFilter;
    ITcpOutgoingFilter*     m_OutgoingFilter;
    CBaseKeepAliveHandler*  m_KeepAliveHandler;

    enum { CREATE_CLOSE_SERVER_SOCKET_TIMEOUT = 5000 };

	bool		m_IsCreated;
	bool		m_MultiClient;
	int			m_ServerPort;
	std::string m_ServerIP;
	int			m_LastConnectionId;
    bool		m_Closing;


	ConnectionsMap m_ConnectionsMap;
    //Eldad- Currently it seems that this map is protecting the class' data structures
    // and the usage of the CTcpSocketData objects that are used from the map
    CCriticalSection    m_MapGuard;
	std::vector<CTcpSocketData*> m_ClosedSockets;

};

