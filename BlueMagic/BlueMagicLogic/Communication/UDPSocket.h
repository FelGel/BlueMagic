#pragma once
#include "BaseBerkeleySocket.h"
#include "Common/LogEvent.h"

class IUDPSocketEvents
{
public:
	virtual void OnUDPData(BYTE* Data, int DataLen, const char* SourceIP, int SourcePort) = 0;
};

class CUDPSocket : public CBaseBerkeleySocket,
                   public IBaseBerkeleySocketEvents
{
public:
	CUDPSocket();
	~CUDPSocket();

    // Returns the port number or -1 in case of failure
	int Create(IUDPSocketEvents* UDPClient, const char* LocalIP = NULL, int ListeningPort = 0);
    using CBaseBerkeleySocket::Close;
	bool SendTo(const void* Data, int DataLength, 
        int DestPort, const char* DestIP,
        ELogSeverity WouldBlockLogLevel = LE_ERROR);

	int MaxDatagramSize()
	{ return m_BufSize; }

    bool IsInitialized() const
        { return IsClosed(); }

private:
    // Hide copy ctor and assignment operator
    CUDPSocket(const CUDPSocket &);
    CUDPSocket & operator=(const CUDPSocket &);
    void AllocateReceiveBuffer();

protected:
    //IBaseBerkeleySocketEvents
    //Notifies a listening socket that there is data to be retrieved by calling Receive(). 
    virtual void OnReceive();
    //Notifies a socket that it can send data by calling Send()
    virtual void OnSend();
    //Not to be used in UDP.
    virtual void OnConnect(int ErrorCode);
    virtual void OnAccept();
    virtual void OnConnectionClosed(int ErrorCode); 

    // overrides
    virtual void OnTimeout();


private:
    //filter burst of socket error messages 
    void HandleUDPSocketError(int ErrorCode, const char* ContextStr);

private:
	IUDPSocketEvents* m_Client;

	BYTE*	m_Buf;
	int		m_BufSize;

    //parameters for filtering log events that occur many times per seconds.
    int     m_LastErrorCode;
    DWORD   m_LastErrorCodeTime;
    enum { DISPLAY_ERROR_FREQUENCY = 2000 };
};
