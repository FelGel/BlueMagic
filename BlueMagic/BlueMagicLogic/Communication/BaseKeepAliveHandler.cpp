#include "StdAfx.h"
#include "BaseKeepAliveHandler.h"
#include "Common/LogEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBaseKeepAliveHandler::CBaseKeepAliveHandler():
m_Sink(NULL)
{
}

CBaseKeepAliveHandler::~CBaseKeepAliveHandler()
{
}


void CBaseKeepAliveHandler::Advise(IKeepAliveHandlerEvents* Sink)
{
    m_Sink = Sink;
}

void CBaseKeepAliveHandler::SendDisconnectionEvent()
{
    if(m_Sink)
        m_Sink->OnKeepAliveDisconnection();
}

bool CBaseKeepAliveHandler::SendKeepAliveMsg(const BYTE* KeepAliveMsgData, int DataSize)
{
    if(m_Sink == NULL)
	{
		LogEvent(LE_WARNING, "CBaseKeepAliveHandler::SendKeepAliveMsg() Sink is NULL");
		return true;
	}
       
	return m_Sink->OnSendKeepAliveMsg(KeepAliveMsgData, DataSize);
}
