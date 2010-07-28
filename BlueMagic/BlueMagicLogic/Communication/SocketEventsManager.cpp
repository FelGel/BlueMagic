#include "StdAfx.h"
#include "SocketEventsManager.h"
#include "BerkeleySocketsUtils.h"
#include "Common/Utils.h"
#include "Common/CollectionHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////
// CSocketEventsManager class
//////////////////////////////////////////////////////////////////////////
bool CSocketEventsManager::g_IsInitialized = false;

//////////////////////////////////////////////////////////////////////////
CSocketEventsManager::CSocketEventsManager()
: CSimpleThread("CSocketEventsManager")
, m_SelectErrorPeriodicLog(LOG_REPEATED_ERROR_IN_SELECT_RATE)
{
    StartThread();
    SetThreadPriority(DEFAULT_THREAD_PRIORITY);
    LogEvent(LE_INFOHIGH, "CSocketEventsManager::CSocketEventsManager, Thread Started");
    StartSocketLibrary();
    g_IsInitialized = true;
}


//////////////////////////////////////////////////////////////////////////
CSocketEventsManager::~CSocketEventsManager()
{
	if (g_IsInitialized)
    {
        g_IsInitialized = false;
        CloseThread(true);
		EndSocketLibrary();
    }
}


//////////////////////////////////////////////////////////////////////////
CSocketEventsManager* CSocketEventsManager::GetTheSocketEventsManager()
{
    static CSocketEventsManager TheSocketEventsManager;

    if (g_IsInitialized == false)
        return NULL;
    return &TheSocketEventsManager;
}


////////////////////////////////////////////////////////////////////
//void CSocketEventsManager::CheckSocketsInitialized()
//{
//    // The m_IsInitialized is protected in order to prevent situation where it is
//    // being initialized from 2 different threads concurrently
//	if (!InterlockedCompareExchange(&m_IsInitialized, TRUE, FALSE))
//	{
//        StartThread();
//        LogEvent(LE_INFOHIGH, "CSocketEventsManager::CheckSocketsInitialized, Thread Started");
//        StartSocketLibrary();
//	}
//}


////////////////////////////////////////////////////////////////
// This function will initialize the socket library
bool CSocketEventsManager::StartTheCommThread(const char* ThreadPriority)
{
    CSocketEventsManager* EventMgr = GetTheSocketEventsManager();
    if (EventMgr == NULL)
    {
        LogEvent(LE_ERROR, "CSocketEventsManager::StartTheCommThread - can't acquire events manager");
        return false;
    }

    int ThreadPriorityValue = ThreadPriorityStr2Value(ThreadPriority);
    if(!EventMgr->SetThreadPriority(ThreadPriorityValue))
    {
        LogEvent(LE_WARNING, "CSocketEventsManager::StartTheCommThread : Failed to set thread priority to %s.", ThreadPriority);
        return false;
    }
    LogEvent(LE_INFO, "CSocketEventsManager::StartTheCommThread : Thread priority is set to %s.", ThreadPriority);
    return true;
}


//////////////////////////////////////////////////////////////////
//void CSocketEventsManager::InitializeSocketsInfrastucture()
//{
//    StartThread();
//    LogEvent(LE_INFOHIGH, "CSocketEventsManager::InitializeSocketsInfrastucture, Thread Started");
//    StartSocketLibrary();
//}
//
//


//////////////////////////////////////////////////////////////////////////
bool CSocketEventsManager::StartSocketLibrary()
{
#ifdef WIN32
	// In WIN32 - we have to startup the socket services
	WSADATA   WsaData;

	WORD VersionRequested = MAKEWORD(2, 0);
	int Result = WSAStartup(VersionRequested, &WsaData);
	if (Result != 0)
	{
		LogEvent(LE_ERROR, "CSocketEventsManager::StartSocketLibrary() Error [%d] init Socket library", Result);
		return false;
	}
#endif
	return true;
}


//////////////////////////////////////////////////////////////////////////
void CSocketEventsManager::EndSocketLibrary()
{
#ifdef WIN32
	WSACleanup(); // Cleanup window socket services
#endif
}


//////////////////////////////////////////////////////////////////////////
bool CSocketEventsManager::RegisterSocket(SOCKET Socket, ISocketEventsManagerEvents* SocketEventsSink)
{
	CCriticalSectionLocker Lock(m_Lock);

	if (m_SocketsMap.size() == MAX_NUM_OF_SOCKETS)
	{
		LogEvent(LE_WARNING, "CSocketEventsManager::RegisterSocket - Max total socket number[%d] exceeded", MAX_NUM_OF_SOCKETS);
		return false;
	}
	// add the socket to the map
	if (InsertValueToMap(m_SocketsMap, Socket, SocketEventsSink) == false)
	{
		LogEvent(LE_ERROR, "CSocketEventsManager::RegisterSocket - Socket [%d] is already registered.", Socket);
		return false;
	}
	// Now there is at least 1 socket registered, make the thread wait less
	m_TimeoutInMilli = TIMEOUT_IN_MILLI_FOR_SOCKET_EVENTS;

    // adding the socket also to the writer map with false value (not registered to write events)
    CCriticalSectionLocker WriterLock(m_WriteSetLock);
    bool SocketWriteEventValue = false;
    if (InsertValueToMap(m_WriteSocketsMap, Socket, SocketWriteEventValue) == false)
    {
        LogEvent(LE_ERROR, "CSocketEventsManager::RegisterSocket - not able to register Socket [%d] to write socket map", Socket);
        Assert(false); // not suppose to happen after the insertion to the first set succeeded
        return false;
    }

    LogEvent(LE_INFOLOW, "CSocketEventsManager::RegisterSocket - Socket[%x] was registered.", Socket);
	return true;
}


//////////////////////////////////////////////////////////////////////////
bool CSocketEventsManager::UnregisterSocket(SOCKET Socket)
{
	// because of this lock and the usage of this lock in the dispatch events function
	//it is guaranteed that this socket will not receive any events after it had unregistered
	CCriticalSectionLocker Lock(m_Lock);

	bool RetVal = m_SocketsMap.erase(Socket) == 1;
    RemoveSocketFromWriteEvents(Socket); // just in case it is not removed yet

	if(m_SocketsMap.empty() == true)
		m_TimeoutInMilli = TIMEOUT_IN_MILLI_WHEN_NO_SOCKETS;

    CCriticalSectionLocker WriterLock(m_WriteSetLock);
    bool WriterMapStatus = m_WriteSocketsMap.erase(Socket) == 1;
    Assert(WriterMapStatus == RetVal); //expecting to have the same value

    LogEvent(LE_INFOLOW, "CSocketEventsManager::UnregisterSocket - Socket[%x] was unregistered.", Socket);
	return RetVal;
}


//////////////////////////////////////////////////////////////////////////
// It is important in those function (Add / Remove) not to use the 
// main lock and to use the m_WriteSetLock. If using the main lock here
// it could lead to a deadlock in case the client of this class is also
// using a lock.
bool CSocketEventsManager::AddSocketToWriteEvents(SOCKET Socket)
{
    CCriticalSectionLocker Lock(m_WriteSetLock);

    SocketBoolMapIterator Iter = m_WriteSocketsMap.find(Socket);
    if (Iter == m_WriteSocketsMap.end())
        return false;

    Iter->second = true;
    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CSocketEventsManager::RemoveSocketFromWriteEvents(SOCKET Socket)
{
    CCriticalSectionLocker Lock(m_WriteSetLock);

    SocketBoolMapIterator Iter = m_WriteSocketsMap.find(Socket);
    if (Iter == m_WriteSocketsMap.end())
        return false;

    Iter->second = false;
    return true;
}


//////////////////////////////////////////////////////////////////////////
void CSocketEventsManager::OnTimeout()
{
    int NumOfEvents = 0; // number of sockets that had events

    do 
    {
        // Perform the select(), and then dispatch the events that came from the sockets library.
        NumOfEvents = ManageSocketEvents();

        // In any case give time to process timers 
        int NextTimeout;
        ProcessTimers(MAX_TIME_TO_PROCESS_TIMERS, NextTimeout);

    } while(NumOfEvents > 0); // perform this loop until there are no more events on the sockets (And then can rest)

	return;
}


//////////////////////////////////////////////////////////////////////////
// Because of a bug in the operating system(2003 server), if calling closesocket()
// while in select() function there is a leak in the non paged pool of the kernel
// memory. Therefore it is essential that no closesocket() will be made
// with sockets that are being sent to select(). The lock here and in unregister
// class member function should make sure it doesn't happen. Make sure that
// a single lock will include both PrepareSocketSet() & select() to ensure
// no sockets could be unregistered between those calls.
// return value is how many sockets had events.
int CSocketEventsManager::ManageSocketEvents()
{
    CCriticalSectionLocker Lock(m_Lock);

    fd_set ReaderSet;
    fd_set WriterSet;
    // get in all the sockets
    if (PrepareSocketSet(ReaderSet, WriterSet) == false)
        //meaning no socket registered
        return 0;

    // Exceptions is similar to write since we expect exceptions only in operations
    // similar to write operations (Connect in which the write must be set to know if succeeded)
    fd_set ExceptSet = WriterSet;

    timeval TimeOut = {0, 0}; // zero means - return immediately

    int Result =
        // IN WIN32 connect error is reported through ExceptSet
        select(FD_SETSIZE, &ReaderSet, &WriterSet, &ExceptSet, &TimeOut);

    if (Result < 0)
    {
        ManageErrorInSelectFunc();
    }
    else if (Result > 0)
    {
        DispatchAllEvents(ReaderSet, WriterSet, ExceptSet, Result);
    }

    return Result;
}


//////////////////////////////////////////////////////////////////////////
bool CSocketEventsManager::PrepareSocketSet(fd_set& GeneralSet, fd_set& WriteSocketsSet)
{
    CCriticalSectionLocker Lock(m_Lock);

    // preparing the general set
    GeneralSet.fd_count = m_SocketsMap.size();
    SocketEventsMapIterator Iter = m_SocketsMap.begin();
    SocketEventsMapIterator End = m_SocketsMap.end();
    for (int i = 0 ; Iter != End ; ++Iter, ++i)
        GeneralSet.fd_array[i] = Iter->first;

    // preparing the write set
    CCriticalSectionLocker WriteLock(m_WriteSetLock); // locking the use of the writer set
    SocketBoolMapIterator WriteIter = m_WriteSocketsMap.begin();
    SocketBoolMapIterator WriteIterEnd = m_WriteSocketsMap.end();
    int Count = 0;
    for ( ; WriteIter != WriteIterEnd ; ++WriteIter)
    {
        if (WriteIter->second)
        {
            WriteSocketsSet.fd_array[Count] = WriteIter->first;
            Count++;
        }
    }
    WriteSocketsSet.fd_count = Count; // the total number of the socket copied to the array

    return GeneralSet.fd_count > 0;
}


//////////////////////////////////////////////////////////////////////////
void CSocketEventsManager::DispatchAllEvents(const fd_set& ReaderSet,
											 const fd_set& WriterSet,
											 const fd_set& ExceptSet,
											 int TotalNumOfSocketsSet)
{
	// Locking the dispatching of the events in order not to dispatch event
	// which socket was closed(unregistered)
	CCriticalSectionLocker Lock(m_Lock);

	DispatchEventType(ReaderSet, EVENT_TYPE_READ, TotalNumOfSocketsSet);
	DispatchEventType(WriterSet, EVENT_TYPE_WRITE, TotalNumOfSocketsSet);
	DispatchEventType(ExceptSet, EVENT_TYPE_EXCEPTION, TotalNumOfSocketsSet);
	return;
}


//////////////////////////////////////////////////////////////////////////
void CSocketEventsManager::DispatchEventType(const fd_set& SocketSet, EEventType Type,
											 int& TotalNumOfResultSockets)
{
    int SetCount = SocketSet.fd_count;
    for (int i = 0 ; i < SetCount ; i++)
    {
        SOCKET Socket = SocketSet.fd_array[i];
        SocketEventsMapIterator Iter = m_SocketsMap.find(Socket);
        if (Iter == m_SocketsMap.end())
        {
            LogEvent(LE_ERROR, "CSocketEventsManager::DispatchEventType - Socket[%x] is not found in the map.",
                        Socket);
            continue;
        }

        // found the socket trigger event
        TotalNumOfResultSockets--;// One socket less in total calculation
        ISocketEventsManagerEvents* EventsSink = Iter->second;
        if (EventsSink == NULL)
        {
            LogEvent(LE_ERROR, "CSocketEventsManager::DispatchEventType - events are null");
            Assert(false);
            continue;
        }
        switch (Type)
        {
        case EVENT_TYPE_READ:
            EventsSink->OnReceiveEvent();
            break;
        case EVENT_TYPE_WRITE:
            EventsSink->OnSendEvent();
            break;
        case EVENT_TYPE_EXCEPTION:
            EventsSink->OnConnectFailed();
            break;
        default:
            Assert(false);
            break;
        }
    }// for
	return;
}


//////////////////////////////////////////////////////////////////////////
void CSocketEventsManager::ManageErrorInSelectFunc()
{
    int ErrorOccured = WSAGetLastError();
    ELogSeverity Severity = LE_ERROR;

    // Logging periodically, and on every time the error is not like the last error
    if (m_LastErrorInSelect != ErrorOccured)
    {
        m_LastErrorInSelect = ErrorOccured;
        m_SelectErrorPeriodicLog.Reset();
    }
    m_SelectErrorPeriodicLog.LogEvent(Severity, "CSocketEventsManager::ManageErrorInSelectFunc() - error [%s]", GetSocketErrorStr(ErrorOccured).c_str());
    return;
}
