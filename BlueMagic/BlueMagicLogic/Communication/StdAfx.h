// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifndef WINVER
#define WINVER 0x0501
#endif

#define _USE_32BIT_TIME_T

#include <afx.h>
#include <afxwin.h>

// TODO: reference additional headers your program requires here
// this will ensure the max sockets size in select() function is more than the default 64 sockets
#define MAX_NUM_OF_SOCKETS 256
// if included from other application that had included winsock2. will not affect our include.
#ifndef FD_SETSIZE
#define FD_SETSIZE  MAX_NUM_OF_SOCKETS
#endif
#include <Winsock2.h>
