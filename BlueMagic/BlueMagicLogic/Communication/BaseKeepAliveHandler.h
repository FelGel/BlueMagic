#pragma once

class IKeepAliveHandlerEvents
{
public:
    virtual void OnKeepAliveDisconnection() = 0;
    virtual bool OnSendKeepAliveMsg(const BYTE* KeepAliveMsgData, int DataSize) = 0; //classes that implement this function should return true only if the message was successfully sent
};

class CBaseKeepAliveHandler
{
public:
    CBaseKeepAliveHandler();
    virtual ~CBaseKeepAliveHandler();

    void Advise(IKeepAliveHandlerEvents* Sink);

    //handle incoming keep alive messages, return if it was a keep alive message
    virtual bool HandleKeepAliveMessage(const BYTE* Data, int DataSize) = 0;

    //should be called by the client in order to let this object to decide 
    //if it needs to disconnect or send keep alive
    virtual void OnTimeout() = 0;

    //should be called by the client in order to reset keep alive
    //counters, for example in case of reconnection.
    virtual void Reset() = 0;

    //the client should call this function in order to 
    //calculate the frequency that it needs to call the OnTimeout() method,
    //return value is in milliseconds
    virtual DWORD TimeoutFrequency() = 0;

    //For server socket, in order to be able to create keep alive handlers for
    //accepted sockets.
    virtual CBaseKeepAliveHandler* Clone() = 0;

protected:
    //functions for dispatching events to the sink interface, 
    //to be used by concrete derived keep alive handlers
    void SendDisconnectionEvent();
    bool SendKeepAliveMsg(const BYTE* KeepAliveMsgData, int DataSize);

private:
    IKeepAliveHandlerEvents* m_Sink;
};