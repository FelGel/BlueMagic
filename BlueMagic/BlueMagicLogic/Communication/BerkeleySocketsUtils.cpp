#include "StdAfx.h"
#include "BerkeleySocketsUtils.h"
#include "Common/Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////
//Utility functions
//////////////////////////////////////////////////////////////////////////

// #define RETURN_STR(ErrorCodeVal) \
//     case ErrorCodeVal: \
//     { \
//         static std::string ErrorStr = std::string(#ErrorCodeVal) + ":" + GetSystemErrorString(ErrorCode); \
//         return ErrorStr.c_str(); \
//     }
// #define RETURN_STR(ErrorCodeVal, ErrorCodeStr) \
//     case ErrorCodeVal:   \
//         return #ErrorCodeVal ": " ErrorCodeStr;
#define RETURN_STR(ErrorCodeVal) \
    case ErrorCodeVal:   \
        return std::string(#ErrorCodeVal) + ":" + GetSystemErrorString(ErrorCode);

//////////////////////////////////////////////////////////////////////////
std::string GetSocketErrorStr(int ErrorCode)
{
    switch(ErrorCode)
    {
        RETURN_STR(WSAEINTR);
        RETURN_STR(WSAEBADF);
        RETURN_STR(WSAEACCES);
        RETURN_STR(WSAEFAULT);
        RETURN_STR(WSAEINVAL);
        RETURN_STR(WSAEMFILE);
        RETURN_STR(WSAEWOULDBLOCK);
        RETURN_STR(WSAEINPROGRESS);
        RETURN_STR(WSAEALREADY);
        RETURN_STR(WSAENOTSOCK);
        RETURN_STR(WSAEDESTADDRREQ);
        RETURN_STR(WSAEMSGSIZE);
        RETURN_STR(WSAEPROTOTYPE);
        RETURN_STR(WSAENOPROTOOPT);
        RETURN_STR(WSAEPROTONOSUPPORT);
        RETURN_STR(WSAESOCKTNOSUPPORT);
        RETURN_STR(WSAEOPNOTSUPP);
        RETURN_STR(WSAEPFNOSUPPORT);
        RETURN_STR(WSAEAFNOSUPPORT);
        RETURN_STR(WSAEADDRINUSE);
        RETURN_STR(WSAEADDRNOTAVAIL);
        RETURN_STR(WSAENETDOWN);
        RETURN_STR(WSAENETUNREACH);
        RETURN_STR(WSAENETRESET);
        RETURN_STR(WSAECONNABORTED);
        RETURN_STR(WSAECONNRESET);
        RETURN_STR(WSAENOBUFS);
        RETURN_STR(WSAEISCONN);
        RETURN_STR(WSAENOTCONN);
        RETURN_STR(WSAESHUTDOWN);
        RETURN_STR(WSAETOOMANYREFS);
        RETURN_STR(WSAETIMEDOUT);
        RETURN_STR(WSAECONNREFUSED);
        RETURN_STR(WSAELOOP);
        RETURN_STR(WSAENAMETOOLONG);
        RETURN_STR(WSAEHOSTDOWN);
        RETURN_STR(WSAEHOSTUNREACH);
        RETURN_STR(WSAENOTEMPTY);
        RETURN_STR(WSAEPROCLIM);
        RETURN_STR(WSAEUSERS);
        RETURN_STR(WSAEDQUOT);
        RETURN_STR(WSAESTALE);
        RETURN_STR(WSAEREMOTE);
        RETURN_STR(WSASYSNOTREADY);
        RETURN_STR(WSAVERNOTSUPPORTED);
        RETURN_STR(WSANOTINITIALISED);
        RETURN_STR(WSAEDISCON);

    default:
        {
//             static std::string ErrorStr;
//             ErrorStr = GetSystemErrorString(ErrorCode);
//             return ErrorStr.c_str();
            return GetSystemErrorString(ErrorCode);
        }
    }

//     switch(ErrorCode)
//     {
//         RETURN_STR(WSAEINTR, "A blocking operation was interrupted by a call to WSACancelBlockingCall.");
//         RETURN_STR(WSAEBADF, "The file handle supplied is not valid.");
//         RETURN_STR(WSAEACCES, "The requested address is a broadcast address, but the appropriate flag was not set.");
//         RETURN_STR(WSAEFAULT, "The lpSockAddrLen argument is not large enough.");
//         RETURN_STR(WSAEINVAL, "The socket has not been bound to an address with Bind.");
//         RETURN_STR(WSAEMFILE, "No more file descriptors are available.");
//         RETURN_STR(WSAEWOULDBLOCK, "The socket is marked as nonblocking and the requested operation would block.");
//         RETURN_STR(WSAEINPROGRESS, "A blocking Windows Sockets operation is in progress.");
//         RETURN_STR(WSAEALREADY, "An operation was attempted on a non-blocking socket that already had an operation in progress.");
//         RETURN_STR(WSAENOTSOCK, "The descriptor is not a socket");
//         RETURN_STR(WSAEDESTADDRREQ, "A destination address is required");
//         RETURN_STR(WSAEMSGSIZE, "The socket is of type SOCK_DGRAM, and the datagram is larger than the maximum supported by the Windows Sockets implementation.");
//         RETURN_STR(WSAEPROTOTYPE, "The specified port is the wrong type for this socket.");
//         RETURN_STR(WSAENOPROTOOPT, "An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.");
//         RETURN_STR(WSAEPROTONOSUPPORT, "The specified port is not supported.");
//         RETURN_STR(WSAESOCKTNOSUPPORT, "The specified socket type is not supported in this address family.");
//         RETURN_STR(WSAEOPNOTSUPP, "MSG_OOB was specified, but the socket is not of type SOCK_STREAM.");
//         RETURN_STR(WSAEPFNOSUPPORT, "The protocol family has not been configured into the system or no implementation for it exists.");
//         RETURN_STR(WSAEAFNOSUPPORT, "The specified address family is not supported.");
//         RETURN_STR(WSAEADDRINUSE, "The specified address is already in use.");
//         RETURN_STR(WSAEADDRNOTAVAIL, "The specified address is not available from the local machine.");
//         RETURN_STR(WSAENETDOWN, "The Windows Sockets implementation has detected that the network subsystem has failed.");
//         RETURN_STR(WSAENETUNREACH, "The network cannot be reached from this host at this time.");
//         RETURN_STR(WSAENETRESET, "The connection must be reset because the Windows Sockets implementation dropped it.");
//         RETURN_STR(WSAECONNABORTED, "The virtual circuit was aborted due to timeout or other failure.");
//         RETURN_STR(WSAECONNRESET, "The virtual circuit was reset by the remote side. If this is a UDP-datagram socket this error would indicate that a previous send operation resulted in an ICMP \"Port Unreachable\" message.");
//         RETURN_STR(WSAENOBUFS, "No buffer space is available. The socket cannot be created.");
//         RETURN_STR(WSAEISCONN, "The socket is already connected.");
//         RETURN_STR(WSAENOTCONN, "The socket is not connected (SOCK_STREAM only).");
//         RETURN_STR(WSAESHUTDOWN, "The socket has been shut down; it is not possible to call SendTo on a socket after ShutDown has been invoked with nHow set to 1 or 2.");
//         RETURN_STR(WSAETOOMANYREFS, "Too many references to some kernel object.");
//         RETURN_STR(WSAETIMEDOUT, "The attempt to connect timed out without establishing a connection.");
//         RETURN_STR(WSAECONNREFUSED, "The attempt to connect was forcefully rejected.");
//         RETURN_STR(WSAELOOP, "Cannot translate name.");
//         RETURN_STR(WSAENAMETOOLONG, "Name component or name was too long.");
//         RETURN_STR(WSAEHOSTDOWN, "A socket operation failed because the destination host was down.");
//         RETURN_STR(WSAEHOSTUNREACH, "A socket operation was attempted to an unreachable host.");
//         RETURN_STR(WSAENOTEMPTY, "Cannot remove a directory that is not empty.");
//         RETURN_STR(WSAEPROCLIM, "A Windows Sockets implementation may have a limit on the number of applications that may use it simultaneously.");
//         RETURN_STR(WSAEUSERS, "Ran out of quota.");
//         RETURN_STR(WSAEDQUOT, "Ran out of disk quota.");
//         RETURN_STR(WSAESTALE, "File handle reference is no longer available.");
//         RETURN_STR(WSAEREMOTE, "Item is not available locally.");
//         RETURN_STR(WSASYSNOTREADY, "WSAStartup cannot function at this time because the underlying system it uses to provide network services is currently unavailable.");
//         RETURN_STR(WSAVERNOTSUPPORTED, "Windows Sockets version requested is not supported.");
//         RETURN_STR(WSANOTINITIALISED, "A successful WSAStartup() must occur before using this API.");
//         RETURN_STR(WSAEDISCON, "Returned by WSARecv or WSARecvFrom to indicate the remote party has initiated a graceful shutdown sequence.");
// 
//     default:
//         {
//             static char UnknownStr[200];
//             sprintf(UnknownStr, "GetSocketErrorStr() Unknown error code [%d].", ErrorCode);
//             return UnknownStr;
//         }
//     }
}

//////////////////////////////////////////////////////////////////////////
// Some ErrorCodes have different meaning in different context
std::string GetConnectErrorStr(int ErrorCode)
{
    switch(ErrorCode)
    {
        RETURN_STR(WSANOTINITIALISED);
        RETURN_STR(WSAENETDOWN);
        RETURN_STR(WSAEADDRINUSE);
        RETURN_STR(WSAEINPROGRESS);
        RETURN_STR(WSAEADDRNOTAVAIL);
        RETURN_STR(WSAEAFNOSUPPORT);
        RETURN_STR(WSAECONNREFUSED);
        RETURN_STR(WSAEDESTADDRREQ);
        RETURN_STR(WSAEFAULT);
        RETURN_STR(WSAEINVAL);
        RETURN_STR(WSAEISCONN);
        RETURN_STR(WSAEMFILE);
        RETURN_STR(WSAENETUNREACH);
        RETURN_STR(WSAENOBUFS);
        RETURN_STR(WSAEALREADY);
        RETURN_STR(WSAENOTSOCK);
        RETURN_STR(WSAETIMEDOUT);
        RETURN_STR(WSAEWOULDBLOCK);
        RETURN_STR(WSAEHOSTUNREACH);
    default:
        {
//             static std::string ErrorStr;
//             ErrorStr = GetSystemErrorString(ErrorCode);
//             return ErrorStr.c_str();
            return GetSystemErrorString(ErrorCode);
        }
    }

//     switch(ErrorCode)
//     {
//         RETURN_STR(WSANOTINITIALISED, "A successful WSAStartup() must occur before using this API."); 
//         RETURN_STR(WSAENETDOWN, "The Windows Sockets implementation has detected that the network subsystem has failed.");
//         RETURN_STR(WSAEADDRINUSE, "The specified address is already in use.");
//         RETURN_STR(WSAEINPROGRESS, "A blocking Windows Sockets call is in progress.");
//         RETURN_STR(WSAEADDRNOTAVAIL, "The specified address is not available from the local machine.");
//         RETURN_STR(WSAEAFNOSUPPORT, "Addresses in the specified family cannot be used with this socket."); 
//         RETURN_STR(WSAECONNREFUSED, "The attempt to connect was forcefully rejected.");
//         RETURN_STR(WSAEDESTADDRREQ, "A destination address is required.");
//         RETURN_STR(WSAEFAULT, "The nSockAddrLen argument is incorrect.");
//         RETURN_STR(WSAEINVAL, "Invalid host address.");
//         RETURN_STR(WSAEISCONN, "The socket is already connected.");
//         RETURN_STR(WSAEMFILE, "No more file descriptors are available.");
//         RETURN_STR(WSAENETUNREACH, "The network cannot be reached from this host at this time.");
//         RETURN_STR(WSAENOBUFS, "No buffer space is available. The socket cannot be connected.");
//         RETURN_STR(WSAEALREADY, "An operation was attempted on a nonblocking socket with an operation already in progress.");
//         RETURN_STR(WSAENOTSOCK, "The descriptor is not a socket.");
//         RETURN_STR(WSAETIMEDOUT, "Attempt to connect timed out without establishing a connection.");
//         RETURN_STR(WSAEWOULDBLOCK, "The socket is marked as nonblocking and the connection cannot be completed immediately.");
//         RETURN_STR(WSAEHOSTUNREACH, "Host is Unreachable.");
//     default:
//         {
//             static char UnknownStr[200];
//             sprintf(UnknownStr, "GetConnectErrorStr() Unknown error code [%d].", ErrorCode);
//             return UnknownStr;
//         }
//     }
}

//////////////////////////////////////////////////////////////////////////
void DisplaySocketsError(const char* MethodName, ELogSeverity Severity)
{
    int ErrorCode = WSAGetLastError();
    LogEvent(Severity, "%s: %s", MethodName, GetSocketErrorStr(ErrorCode).c_str());
}

#undef RETURN_STR
