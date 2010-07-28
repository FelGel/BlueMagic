#include "stdafx.h"
#include "UDPSocket.h"
#include "BerkeleySocketsUtils.h"
#include "Common/LogEvent.h"
#include "Common/Utils.h"
#include "Common/TimePeriod.h"
#include "Common/TimeStamp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum
{
	BERKELEY_MAX_UDP_DATAGRAM			= 8192,
    BROADCAST_RECOMENDED_UDP_DATAGRAM	= 512
};

CUDPSocket::CUDPSocket()
: m_Client(NULL)
, m_Buf(NULL)
, m_BufSize(0)
, m_LastErrorCode(0)
, m_LastErrorCodeTime(0)

{
}

CUDPSocket::~CUDPSocket()
{
    Close();
    LogEvent(LE_INFOLOW, "CUDPSocket Destructor");
	if(NULL != m_Buf)
	{
        BYTE * Buf = m_Buf;
        m_Buf = NULL;
		delete [] Buf;
	}
}
// Returns the port number or -1 in case of failure
int CUDPSocket::Create(IUDPSocketEvents* UDPClient, const char* LocalIP, int Port)
{
	if(NULL == UDPClient)
		LogEvent(LE_INFO, "CUDPSocket::Create : The sink client is NULL, data can only be sent");
	m_Client = UDPClient;
    if(!CBaseBerkeleySocket::Create(Port, LocalIP, CBaseBerkeleySocket::UDP_SOCKET, this))
	{
		DisplaySocketsError("CUDPSocket::Create");
		return -1;
	}
    std::string SocketAddress;
	int SocketPort;
	if(!GetSockName(SocketAddress, SocketPort))
	{
		DisplaySocketsError("CUDPSocket::Create");
		return -1;
	}

    LogEvent(LE_INFOLOW, "CUDPSocket::Create : Created a new UDP socket. Port#%d",
		SocketPort);

	return SocketPort;
}

void CUDPSocket::AllocateReceiveBuffer()
{
    if (m_Buf != NULL)
        return;

    unsigned int MaxMsgSize = 0;
    if(GetSockOptMaxMessageSize(MaxMsgSize))
    {
        m_BufSize = MaxMsgSize;
    }
    else
    {
        DisplaySocketsError("CUDPSocket::AllocateReceiveBuffer");
        m_BufSize = BERKELEY_MAX_UDP_DATAGRAM;
    }

    m_Buf = new BYTE[m_BufSize];
    LogEvent(LE_INFO, "CUDPSocket::AllocateReceiveBuffer : m_BufSize %d, m_Buf %p", m_BufSize, m_Buf);
}

bool CUDPSocket::SendTo(const void* Data, int DataLength, 
                        int DestPort, const char* DestIP,
                        ELogSeverity WouldBlockLogLevel)
{	
    //TODO: maybe make this functionality inside the berkeley socket
	int BytesSent = CBaseBerkeleySocket::SendTo(Data, DataLength, DestIP, DestPort);
	if(SOCKET_ERROR == BytesSent)
	{
		int ErrorCode = GetLastError();
		if(WSAEWOULDBLOCK == ErrorCode)
		{
			LogEvent(WouldBlockLogLevel, "UDPSocket::SentTo : received blocked error. ");
		}
        else
		    HandleUDPSocketError(ErrorCode, "CUDPSocket::SendTo()");
		return false;
	}
    else if (BytesSent < DataLength)
    {
        LogEvent(LE_WARNING, "CUDPSocket::SendTo - not all the data[%d] was sent to [%s:%d]",
                    DataLength, DestIP, DestPort);
        return false;
    }
	return true;
}

void CUDPSocket::HandleUDPSocketError(int ErrorCode, const char* ContextStr)
{
    //the following code will filter the display of incoming errors 
    if(ErrorCode != m_LastErrorCode || 
        GetTickCount()- m_LastErrorCodeTime > DISPLAY_ERROR_FREQUENCY)
    {
        m_LastErrorCode = ErrorCode;
        m_LastErrorCodeTime = GetTickCount();
        DisplaySocketsError(ContextStr, LE_WARNING);
    }
}

void CUDPSocket::OnReceive()
{
    AllocateReceiveBuffer();

    std::string SenderAddress = "";
	int SourcePort = 0;
	int BytesRead = ReceiveFrom((char*)m_Buf, m_BufSize, SenderAddress, SourcePort);
	switch(BytesRead)
	{
	case 0:
		LogEvent(LE_WARNING, "CUDPSocket::OnReceive : The connection has been closed before data could be read");
		break;

	case SOCKET_ERROR:
        HandleUDPSocketError(GetLastError(), "CUDPSocket::OnReceive (SOCKET_ERROR)");
		break;

	default:
		m_Client->OnUDPData(m_Buf, BytesRead, SenderAddress.c_str(), SourcePort);
		break;
	}
}

void CUDPSocket::OnSend()
{
    std::string SocketAddress;
    int SocketPort;
    if(!GetSockName(SocketAddress, SocketPort))
    {
        LogEvent(LE_INFO, "CUDPSocket::OnSend() : GetSockName failed");
        return;
    }

    //LogEvent(LE_INFOLOW, "CUDPSocket::OnSend() : Socket (with port [%d]) ready for sending", SocketPort );
}

void CUDPSocket::OnConnect(int nErrorCode)
{
    //TODO: is this function is ever get used?
    Assert(false);

    if(0 != nErrorCode)
        DisplaySocketsError("CUDPSocket::OnConnect()"/*nErrorCode*/);
    else
        LogEvent(LE_INFO, "CUDPSocket::OnConnect() : No error");
}

void CUDPSocket::OnAccept()
{
    //should not be called
    Assert(false);
}

void CUDPSocket::OnConnectionClosed(int ErrorCode)
{
    ErrorCode;
    //should not be called
    Assert(false);
}

void CUDPSocket::OnTimeout()
{
    // do nothing.
}

//////////////////////////////////////////////////////////////////////////
///			UDPSocket Tests
//////////////////////////////////////////////////////////////////////////
#ifdef _TEST

class CUDPClient : public IUDPSocketEvents
{
public:
	virtual void OnUDPData(BYTE* Data, int DataLen, const char* SourceIP, int SourcePort);
};

void CUDPClient::OnUDPData(BYTE* /*Data*/, int DataLen,
						   const char* SourceIP, int SourcePort)
{
	static int i = 0;
    // These parameters for testing if there is a big gap between the received packets
    static DWORD LastBufferReceived = GetTickCount();
    const DWORD AllowedGapBetweenPackets = 40;

    if (GetTickCount() - LastBufferReceived > AllowedGapBetweenPackets)
        LogEvent(LE_WARNING, "CUDPClient::OnUDPData - Too big a gap[%d milli] between this packet and the one before it",
                            GetTickCount() - LastBufferReceived);

    LastBufferReceived = GetTickCount();
	LogEvent(LE_INFOLOW, "CUDPClient::OnUDPData : Received %d bytes from %s:%d. Packet#%d",
												DataLen, SourceIP, SourcePort, i++);
}

class CUDPSocketTester
{
public:
	bool TestCreate();
	bool TestSendTo();
    void TestReceiveFrom();
	void TestTimer();
private:
	CUDPClient m_UDPClient;
    CUDPSocket m_UDPSocket;
    static const int m_TestPort = 5004;
};

bool CUDPSocketTester::TestCreate()
{
	int UDPPort = m_UDPSocket.Create(&m_UDPClient, NULL, m_TestPort);
    //WaitAndPumpMessages(1000); // it seems that this is needed in order for the sockets library to load for receive
	return m_TestPort == UDPPort;
}

bool CUDPSocketTester::TestSendTo()
{
	const int DataLength = 512;
	char Data[DataLength];
	int NumPackets = 1000;
	char* DestIP = "127.0.0.1";
	for(int i = 0; i < NumPackets; ++i)
	{
		//WaitAndPumpMessages(0); // Delay to prevent the socket from blocking and to enable receive
		if(!m_UDPSocket.SendTo(Data, DataLength, m_TestPort, DestIP))
		{
			LogEvent(LE_ERROR, "CUDPSocketTester::TestSendTo : failed to send buf#%d", i);
			//return false;
		}
		else
			LogEvent(LE_DEBUG, "CUDPSocketTester::TestSendTo : sent %d bytes to %s:%d. Packet#%d",
												DataLength, DestIP, m_TestPort, i);
	}

	return true;
}

void CUDPSocketTester::TestReceiveFrom()
{
    const int ReceiveBufferSize = 65536;
    Assert(m_UDPSocket.SetSockOptReceiveBuffSize(ReceiveBufferSize) == true);

    LogEvent(LE_INFOHIGH, "CUDPSocketTester::TestReceiveFrom() - Test started on port [%d], received packets logs should be written now(info low)", m_TestPort);


    WaitAndPumpMessages(100000);
    return;
}

void TestUDPSocket()
{
    bool Success;
    CUDPSocketTester Tester;

    Success = Tester.TestCreate();
    LogEvent(LE_INFO, "TestUDPSocket : TestCreate %s", Success ? "Succeeded" : "Failed");
    Success = Tester.TestSendTo();
    LogEvent(LE_INFO, "TestUDPSocket : TestSendTo %s", Success ? "Succeeded" : "Failed");
    Tester.TestReceiveFrom();
    LogEvent(LE_INFOHIGH, "TestUDPSocket : TestReceiveFrom was finished");

    return;
}

#endif