#pragma once


//////////////////////////////////////////////////////////////////////////
// The ITcpOutgoingFilter and ITcpIncomingFilter are extension interfaces to the
// CTcpSocketService. Their goal is to provide an extra processing layer of 
// outgoing and incoming messages (for example, to support message wrappers). 
// A TcpServiceSocket client can provide a concrete filters according to context.
// 
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// TcpOutgoingFilter

class ITcpOutgoingFilterEvents
{
public:
    enum ESendStatus
    {
        SEND_STATUS_OK, //send success
        SEND_STATUS_BLOCKED, //retry
        SEND_STATUS_FAILED //abort message
    };
    virtual ESendStatus OnOutgoingFilterMessage(const void* Data, int DataSize) = 0;
};

class ITcpOutgoingFilter
{
public:
    virtual ~ITcpOutgoingFilter() {}
    virtual void AdviseOutgoingFilterEvents(ITcpOutgoingFilterEvents* Sink) = 0;
    virtual bool SendMessage(const void* Data, int DataSize, bool IsPriorityMsg = true) = 0;
    virtual void Reset() = 0;
    virtual ITcpOutgoingFilter* CloneOutgoingFilter() = 0;
};

class CBaseTcpOutgoingFilter: public ITcpOutgoingFilter
{
public:
    CBaseTcpOutgoingFilter();
    virtual void AdviseOutgoingFilterEvents(ITcpOutgoingFilterEvents* Sink);

protected:
    ITcpOutgoingFilterEvents::ESendStatus SendOutgoingFilterMessage(
        const void* Data, int DataSize);

private:
    ITcpOutgoingFilterEvents* m_Sink;
};

//////////////////////////////////////////////////////////////////////////
// TcpIncomingFilter

class ITcpIncomingFilterEvents
{
public:
    virtual void OnIncomingFilterMessage(const void* Data, int DataSize) = 0;
};

class ITcpIncomingFilter
{
public:
    virtual ~ITcpIncomingFilter() {}
    virtual void AdviseIncomingFilterEvents(ITcpIncomingFilterEvents* Sink) = 0;
    virtual void OnIncomingMessage(const void* Data, int DataSize) = 0;
    virtual void Reset() = 0;
    virtual ITcpIncomingFilter* CloneIncomingFilter() = 0;
};

class CBaseTcpIncomingFilter: public ITcpIncomingFilter
{
public:
    CBaseTcpIncomingFilter();
    virtual void AdviseIncomingFilterEvents(ITcpIncomingFilterEvents* Sink);

protected:
    void SendIncomingFilterMessage(const void* Data, int DataSize);

private:
    ITcpIncomingFilterEvents* m_Sink;
};


//////////////////////////////////////////////////////////////////////////
// A Base Class for simple parsers that accumulate incoming data in
// internal buffer and try to parse the data. A concrete derived class
// should implement the ParseMessage() function to decide if the accumulated
// data contains message.
// A concrete derived class should implement the CloneIncomingFilter() function
// as well.
// In case that the internal buffer becomes full and not parsed the Parser will
// clear the buffer and add error to the log.

#include <afxmt.h>
class CBasicTcpParser: public CBaseTcpIncomingFilter
{
public:
    CBasicTcpParser(int DataBufferSize);
    ~CBasicTcpParser();

    //implementation of CBaseTcpIncomingFilter
    virtual void OnIncomingMessage(const void* Data, int DataSize);
    virtual void Reset();

private:
    // Hide copy ctor and assignment operator
    CBasicTcpParser(const CBasicTcpParser &);
    CBasicTcpParser & operator=(const CBasicTcpParser &);

protected:
    //to be implemented by derived classes
    virtual bool ParseMessage(const BYTE* Data, int DataSize, int& ActualParsed) = 0; //return the actual parsed data
    virtual void HandleParsedMessage(const BYTE* Data, int DataSize); // default implementation will be to pass the parsed message to the sink

private:
    void ParseInternalData();
    void DeleteDataBuffer();
    void AllocateInternalBuffer(int DataSize);

private:
    CCriticalSection    m_DataLock;
    BYTE*               m_Data;
    int                 m_DataSize;
    int                 m_ActualSize;
};

