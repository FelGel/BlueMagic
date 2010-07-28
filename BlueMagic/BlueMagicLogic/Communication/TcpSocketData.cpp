// TcpSocketData.cpp: implementation of the CTcpSocketData class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TcpSocketServer.h"
#include "Common\LogEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// class CTcpSocketData


CTcpSocketData::CTcpSocketData()
: m_ConnectionHandle(-1), m_Sink(NULL)
{
}

CTcpSocketData::~CTcpSocketData()
{
}

bool CTcpSocketData::SendMessage(const void* Data, int DataSize, bool IsPriorityMsg,
								 ELogSeverity WouldBlockLogLevel, int NumWouldBlockResends)
{
	return m_Socket.SendMessage(Data, DataSize, IsPriorityMsg, WouldBlockLogLevel, NumWouldBlockResends);
}

void CTcpSocketData::Init(int ConnectionHandle, 
						  ITcpSocketDataEvents *Sink, 
						  SOCKET ConnectedSocket,	
                          ITcpIncomingFilter* IncomingFilter,
                          ITcpOutgoingFilter* OutgoingFilter,
                          CBaseKeepAliveHandler* KeepAliveHandler)
{
	m_ConnectionHandle = ConnectionHandle;
	m_Sink = Sink;
	m_Socket.Create(ConnectedSocket, true, this);
    m_Socket.SetKeepAliveHandler(KeepAliveHandler);
    m_Socket.SetFilters(OutgoingFilter, IncomingFilter);
    m_Socket.EnableKeepAlive(true); //socket is connected
}

void CTcpSocketData::Advise(ITcpSocketDataEvents* Sink)
{
	if(m_Sink && Sink != NULL)
		LogEvent(LE_INFOLOW, "TcpSocketData(%s)::Advise : overriding existing sink.", m_Context.c_str());
	m_Sink = Sink;
}

void CTcpSocketData::CloseDataConnection()
{
	LogEvent(LE_INFOLOW, "TcpSocketData(%s)::CloseDataConnection", m_Context.c_str());
    m_Socket.EnableKeepAlive(false); 
	m_Socket.Close();
    m_Socket.SetFilters(NULL, NULL);
}
    
// private methods

///////////////////////////////////////////////////////
// ITcpSocketSerivceEvents
///////////////////////////////////////////////////////
/*virtual*/
void CTcpSocketData::OnMessage(const void* Data, int DataSize)
{
	if(m_Sink)
		m_Sink->OnSocketDataMessage(Data, DataSize, m_ConnectionHandle);
	else
		LogEvent(LE_ERROR, "CTcpSocketData(%s)::OnMessage: m_Sink=NULL", m_Context.c_str());
}

/*virtual*/ 
void CTcpSocketData::OnClose()
{
    m_Socket.EnableKeepAlive(false); 
	if(m_Sink)
	{
		LogEvent(LE_INFOLOW, "TcpSocketData(%s)::OnClose : sending disconnection event to client.", m_Context.c_str());
		m_Sink->OnSocketDataDisconnected(m_ConnectionHandle);
	}
	else
		LogEvent(LE_ERROR, "TcpSocketData(%s)::OnClose: m_Sink=NULL", m_Context.c_str());
}

void CTcpSocketData::OnSend()
{
	if(m_Sink)
		m_Sink->OnReadyForSend();
	else
		LogEvent(LE_ERROR, "TcpSocketData(%s)::OnSend : no sink.", m_Context.c_str());
}

bool CTcpSocketData::GetPeerData(std::string& Ip, UINT& Port)
{
	return m_Socket.GetPeerData(Ip, Port);
}

bool CTcpSocketData::GetSockName(std::string& Ip, UINT& Port)
{
	return m_Socket.GetSockName(Ip, Port);
}

bool CTcpSocketData::IsConnected() const
{
    return m_Socket.IsConnected();
}
