#pragma once

#include <string>
#include "Common/LogEvent.h"
                

//////////////////////////////////////////////////////////////////////////
// This file include utilities for Berkeley Socket infrastructure

//utility functions

std::string GetSocketErrorStr(int ErrorCode);
std::string GetConnectErrorStr(int ErrorCode);
void DisplaySocketsError(const char* MethodName, ELogSeverity Severity = LE_ERROR);

#define BERKELEY_SOCKETS_OK 0

