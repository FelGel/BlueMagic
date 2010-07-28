#pragma once

#include "StdAfx.h"
#include "Common/CriticalSection.h"
#include "Common/SimpleThread.h"
#include "Common/TimingCenter.h"
#include "Common/LogEvent.h"
#include <set>

//////////////////////////////////////////////////////////////////////////
// Description:
//This will be a simple thread and will inherit from CTimerCenter to be able to use timers.
//It will not handle any requests but instead its loop function will be dedicated to receiving
//the socket library events.
//Each socket will register in this component for the events it needs.
//The loop function will call select() method of the Window Socket library to find out what
//are the statuses of all the sockets that were registered. This call to select is a blocking call and
//it will be done with a short timeout. When the select returns it will trigger the
//relevant events according the result from the select() method. Those event will be
//transferred to the various sockets.

//////////////////////////////////////////////////////////////////////////
class ISocketEventsManagerEvents
{
public:
	// indicates one of the following:
	// o	If processing a connect call (nonblocking), connection has succeeded. 
	// o	Data can be sent
	virtual void OnSendEvent() = 0;

	//indicate the following : 
	//o	If listen has been called and a connection is pending, accept will succeed. 
	//o	Data is available for reading 
	//o	The remote side had closed the connection.
	virtual void OnReceiveEvent() = 0;

	//If trying to connect the connection had failed. 
	virtual void OnConnectFailed() = 0;
};


//////////////////////////////////////////////////////////////////////////
class CSocketEventsManager : public CSimpleThread,
							 public CTimingCenter
{
public:
    // acquire the singleton instance.
    static CSocketEventsManager* GetTheSocketEventsManager();

    // Call this function in the beginning of the Application only if you want the 
    // sockets library and thread to be initialized always regardless if there are sockets in use.
    // If you want the sockets to be initialized only when actually used, then don't call this function
    static bool StartTheCommThread(const char* ThreadPriority = "HIGHEST");

	//this will register the socket to the events.
	bool RegisterSocket(SOCKET Socket, ISocketEventsManagerEvents* SocketEventsSink);

	//remove this socket from the list of sockets. It is guaranteed that after this function returns
	//there will be no more events triggered for this socket.
	bool UnregisterSocket(SOCKET Socket);

    // Since the write events are needed only when connecting (TCP) - will not
    // register a socket to write events unless it has called this function.
    // This in order to prevent from getting this events too often and loading the CPU.
    bool AddSocketToWriteEvents(SOCKET Socket);
    bool RemoveSocketFromWriteEvents(SOCKET Socket);


private:
    // Singleton
    CSocketEventsManager();
    ~CSocketEventsManager();

	// all the processing will be done in on timeout. The timeout interval
	// will be set to 0, so the thread will not wait before calling
	// timeouts. In this function need to wait on the select function
	virtual void OnTimeout(); //override the implementation of simple thread

	// start & end the windows socket library - should be called only once
	bool StartSocketLibrary();
	void EndSocketLibrary();

    int ManageSocketEvents();
	bool PrepareSocketSet(fd_set& GeneralSet, fd_set& WriteSocketsSet);

	enum EEventType
	{
		EVENT_TYPE_READ,
		EVENT_TYPE_WRITE,
		EVENT_TYPE_EXCEPTION
	};
	void DispatchAllEvents(const fd_set& ReaderSet, const fd_set& WriterSet, const fd_set& ExceptSet,
						   int TotalNumOfResultSockets);
	void DispatchEventType(const fd_set& SocketSet, EEventType Type, int& TotalNumOfResultSockets);

    // Deal with the situation where the select had returned error status
    void ManageErrorInSelectFunc();

	enum
	{
		TIMEOUT_IN_MILLI_WHEN_NO_SOCKETS = 20,
		TIMEOUT_IN_MILLI_FOR_SOCKET_EVENTS = 10,
		MAX_TIME_TO_PROCESS_TIMERS = 20,
        DEFAULT_THREAD_PRIORITY = THREAD_PRIORITY_HIGHEST,
        LOG_REPEATED_ERROR_IN_SELECT_RATE = 1000,
	};

private:
    // Check whether the singleton was deleted or not.
	static bool g_IsInitialized;

	// The lock is to lock the map members and their usage (dispatching events)
	CCriticalSection m_Lock;

    // This lock is especially for the write set map. Since we want to limit
    // the main lock only to API functions of register / unregister(not to limit
    // the client too much with function he can't call while locked with another lock),
    // this lock is necessary.
    CCriticalSection m_WriteSetLock;

	typedef std::map<SOCKET, ISocketEventsManagerEvents*> SocketEventsMap;
	typedef SocketEventsMap::iterator SocketEventsMapIterator;
	// The map to hold all the sockets and their sinks
	SocketEventsMap m_SocketsMap;

    typedef std::map<SOCKET, bool> SocketBoolMap;
    typedef SocketBoolMap::iterator SocketBoolMapIterator;
    // The map is holding only the sockets that want to register also to write events.
    SocketBoolMap m_WriteSocketsMap;

    // Those for not writing too many log events when there is repeated error in select() call
    int m_LastErrorInSelect;
    CPeriodicLogEvent m_SelectErrorPeriodicLog;
};
