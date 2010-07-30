#include "StdAfx.h"
#include "TcpSocketExtensions.h"
#include "Common/LogEvent.h"
#include "Common/CriticalSection.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CBaseTcpOutgoingFilter

CBaseTcpOutgoingFilter::CBaseTcpOutgoingFilter(): m_Sink(NULL)
{
}

void CBaseTcpOutgoingFilter::AdviseOutgoingFilterEvents(ITcpOutgoingFilterEvents* Sink)
{
    m_Sink = Sink;
}

ITcpOutgoingFilterEvents::ESendStatus CBaseTcpOutgoingFilter::SendOutgoingFilterMessage(
    const void* Data, int DataSize)
{
    if(m_Sink == NULL)
    {
        Assert(false);
        return ITcpOutgoingFilterEvents::SEND_STATUS_FAILED;
    }
    return m_Sink->OnOutgoingFilterMessage(Data, DataSize);
}


//////////////////////////////////////////////////////////////////////////
// CBaseTcpIncomingFilter

CBaseTcpIncomingFilter::CBaseTcpIncomingFilter(): m_Sink(NULL)
{
}

void CBaseTcpIncomingFilter::AdviseIncomingFilterEvents(ITcpIncomingFilterEvents* Sink)
{
    m_Sink = Sink;
}

void CBaseTcpIncomingFilter::SendIncomingFilterMessage(const void* Data, int DataSize)
{
    if(m_Sink == NULL)
    {
        Assert(false);
        return;
    }
    return m_Sink->OnIncomingFilterMessage(Data, DataSize);
}


//////////////////////////////////////////////////////////////////////////
// CBasicTcpParser

CBasicTcpParser::CBasicTcpParser(int DataSBufferize): 
    m_Data(NULL), m_DataSize(0), m_ActualSize(0)
{
    AllocateInternalBuffer(DataSBufferize);
}

CBasicTcpParser::~CBasicTcpParser()
{
    DeleteDataBuffer();
}

//implementation of CBaseTcpIncomingFilter
void CBasicTcpParser::OnIncomingMessage(const void* Data, int DataSize)
{
    CCriticalSectionLocker Locker(m_DataLock);

    if (m_Data == NULL)
    {
        LogEvent(LE_WARNING, "CBasicTcpParser OnIncomingMessage: m_Data is NULL, probably called after dtor!");
        return;
    }

    int DataToCopyOffset = 0;
    while(DataToCopyOffset < DataSize)
    {
        int InternalSpaceLeft = m_DataSize - m_ActualSize;
        Assert(InternalSpaceLeft >= 0);
        int SizeToCopy = min(InternalSpaceLeft, DataSize - DataToCopyOffset);
        const BYTE* DataToCopyPtr = ((const BYTE*) Data) + DataToCopyOffset;
        memcpy(m_Data + m_ActualSize, DataToCopyPtr, SizeToCopy);
        m_ActualSize += SizeToCopy;
        DataToCopyOffset += SizeToCopy;

        ParseInternalData();
    }
    Assert(DataToCopyOffset == DataSize);
}

void CBasicTcpParser::Reset()
{
    CCriticalSectionLocker Locker(m_DataLock);
    m_ActualSize = 0;
}

void CBasicTcpParser::ParseInternalData()
{
    int ActualParsed;
    while (ParseMessage(m_Data, m_ActualSize, ActualParsed))
    {
        //			LogEvent(LE_DEBUG,"CTcpSocketService::OnReceive: ActualReceived=%d, m_DataSize=%d m_ActualSize=%d ActualParsed=%d",
        //				ActualReceived, m_DataSize, m_ActualSize, ActualParsed);
        Assert(m_ActualSize > 0);
        Assert(m_ActualSize >= ActualParsed);

        m_ActualSize -= ActualParsed;
        HandleParsedMessage(m_Data, ActualParsed);
        Assert(m_ActualSize >= 0);
        memmove(m_Data, m_Data + ActualParsed, m_ActualSize);
    }

    if(m_ActualSize == m_DataSize)
    {
        //internal data buffer is full and data cannot be parsed:
        //log error and clear the internal buffer
        LogEvent(LE_ERROR, "Unable to Parse Data while internal buffer full (%d), Clear internal buffer", m_ActualSize);
        m_ActualSize = 0;
    }
}

void CBasicTcpParser::HandleParsedMessage(const BYTE* Data, int DataSize)
{
    SendIncomingFilterMessage(Data, DataSize);
}

void CBasicTcpParser::DeleteDataBuffer()
{
    CCriticalSectionLocker Locker(m_DataLock);
    if(m_Data)
    {
        BYTE * Data = m_Data;
        m_Data = NULL;
        m_DataSize = 0;
        delete [] Data;
    }
}

void CBasicTcpParser::AllocateInternalBuffer(int DataSize)
{
    CCriticalSectionLocker Locker(m_DataLock);
    DeleteDataBuffer();
    m_DataSize = DataSize;
    if (m_DataSize > 0)
        m_Data = new BYTE[m_DataSize];
}


