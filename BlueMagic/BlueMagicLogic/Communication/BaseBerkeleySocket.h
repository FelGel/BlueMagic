#pragma once
#include "SocketEventsManager.h"

// Description: This component will be the gateway for the sockets library.
//Any socket action or event is passing through this component.
//It has some public operations (Like send and receive) which are public to be used by
//the clients of the socket library, and also some protected operations (Listen, Accept)
//to be used by derived sockets. In any case there is no direct call to the
//Windows Sockets Library outside this component.
//It will have a CSocketEventsManager as a static member. It will register all the sockets
//in this static member upon creation of the sockets, and all the events will be triggered by this Manager.
//For that reason the CBaseBerkeleySocket will implement the ISocketEventsManagerEvents

//////////////////////////////////////////////////////////////////////////
class IBaseBerkeleySocketEvents
{
public:
	//Notifies a listening socket that there is data to be retrieved by calling Receive(). 
	virtual void OnReceive() = 0;
	//Notifies a socket that it can send data by calling Send()
	virtual void OnSend() = 0;
	//Notifies a listening socket that it can accept pending connection requests by calling Accept.
	virtual void OnAccept() = 0;
	//Notifies a connecting socket that the connection attempt is complete, whether successfully or in error.
	virtual void OnConnect(int ErrorCode) = 0;
	//Notifies a socket that the socket connected to it has closed.
	virtual void OnConnectionClosed(int ErrorCode) = 0; 
};


//////////////////////////////////////////////////////////////////////////
class CBaseBerkeleySocket : public ISocketEventsManagerEvents, 
                            public ITimingCenterEvents
{
public:
	CBaseBerkeleySocket();
	~CBaseBerkeleySocket();

protected:
	enum ESocketType
	{
		UDP_SOCKET,
		TCP_SOCKET
	};
	//Create a socket.
	bool Create(int LocalSocketPort, const char* LocalSocketAdd, ESocketType SocketType,
				IBaseBerkeleySocketEvents* Sink);
    // Create a Berkeley socket from already created socket (From accept() function)
    bool Create(SOCKET AlreadyCreatedSocket, IBaseBerkeleySocketEvents* Sink);
	//Closes the socket. Calls the UnregisterSocket() of the CSocketEventsManager.
	void Close();

	// Connect to a remote TCP socket.
	bool Connect(const char* HostAddress, int HostPort);
	// start listening for incoming TCP connections
	bool Listen();
	// Accepts a connection on the listening socket. Return the new socket that is the result of the connection.
	SOCKET Accept(bool ShouldContinueListening);
	//send data to the destination the socket is connected to. return actual sent.(TCP)
    int Send(const void* Buffer, int BufferLen);
	// Sends data to a specific destination.
	int SendTo(const void* Buffer, int BufferLen, const char* HostAddress, int HostPort);
	// Receives data from the socket.
    int Receive(char* Buffer, int BufferLen);
	// Receives a datagram and stores the source address. This function will work only on non-connected sockets 
    int ReceiveFrom(char* Buffer, int BufferLen, std::string& RemoteIP, int& RemotePort);

public:
	// Gets the address of the peer socket to which the socket is connected. (If connected)
    bool GetPeerName(std::string& PeerIP, int& PeerPort) const;
	// Gets the local name for a socket. 
	bool GetSockName(std::string& SockIP, int& SockPort) const;
	//  Set/Get the socket option SO_MAX_MSG_SIZE
    bool SetSockOptMaxMessageSize(unsigned int MaxMsgSize);
	bool GetSockOptMaxMessageSize(unsigned int& MaxMsgSize) const;
	//  Set/Get the socket option TCP_NODELAY which disables the Nagle algorithm for send coalescing.
	bool SetSockOptNoDelay(bool NoDelay);
    bool GetSockOptNoDelay(bool& NoDelay) const;
	//  Set/Get the option RCVBUF - size of receive buffer
	bool SetSockOptReceiveBuffSize(int Size);
    bool GetSockOptReceiveBuffSize(int& Size) const;
    //  Set/Get the option SNDBUF - size of receive buffer
    bool SetSockOptSendBuffSize(int Size);
    bool GetSockOptSendBuffSize(int& Size) const;
	// Set the socket option  IP_TOS (Type-of-service)
	bool SetSockOptTos(int Tos);
    bool GetSockOptTos(int& Tos) const;
	// add a timer to the CSocketEventsManager.

    bool StartTimer(DWORD TimerRateInMilli, const char* Context);
	//  remove the timer from the CSocketEventsManager.
	bool StopTimer();
	// get the timer rate in milli.
	DWORD GetCurrentTimerRate() const;
    // knows whether the socket is closed or not.
    bool IsClosed() const;

    int GetLastError() const;
    const char * GetLastErrorAsString() const;

    static bool CheckAddressValidity(const char* Ip);

protected:
	//ITimingCenterEvents - the timer function. To be override
    virtual void OnTimeout() { }

private:
    // Disables Send and/or Receive calls on the socket. differs from close() and still need to do close
    bool ShutDown();
    bool Bind(int LocalSocketPort, const char* LocalSocketAdd);
    void SetIsConnectingFlag(bool IsConnceting);
    void CheckingWouldBlockOnSend(int SendReturnStatus);
    // Control whether or not to register to the socket write events (onSendEvent())
    void SetRegistrationToWriteEvents(bool ShouldRegister);
    DWORD CalculateAdressFromIp(const char* RemoteIp);

	//ISocketEventsManagerEvents
	virtual void OnSendEvent();
	virtual void OnReceiveEvent();
	virtual void OnConnectFailed();

private:
    enum
    {
        NULL_TIMER_ID = 0
    };

	// The component in which to get all the events
	//static CSocketEventsManager g_SocketEventsMgr;

	// hold the socket handle.
	SOCKET m_Socket;
    // Sink for socket events
    IBaseBerkeleySocketEvents* m_Sink;

    // Those members are accessed from 2 threads concurrently (Client thread and Socket's thread)
	// If the Listen() function of a socket was called. This is Reset when OnAccept() event is triggered.
	volatile bool m_IsListening;
	// Is the Connect() function was activated. This is reset if the event OnConnect() is triggered
	volatile bool m_IsConnecting;
    // To know whether the last send operation has been would blocked
    volatile bool m_IsLastSendWouldBlock;

    DWORD m_TimerRateInMilli;
    DWORD m_TimerId;

    // since calculate the IP in DWORD format is costly, and normally each time we use it for the
    // same address (send to operation is for the same address, or connect) we calculate the
    // DWORD once and cache it and use it each time instead calling the calculation.
    std::string m_LastCalculatedIp;
    DWORD m_LastDwordAddress;

    mutable std::string m_PeerIp;
    mutable int m_PeerPort;
};

//////////////////////////////////////////////////////////////////////////
enum NetworkHeaderLength
{
    ETHERNET_HEADER_LENGTH  =   14,
    IP_HEADER_LENGTH        =   20,
    UDP_HEADER_LENGTH       =    8,
};
