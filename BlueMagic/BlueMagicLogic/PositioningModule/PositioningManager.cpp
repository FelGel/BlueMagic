#include "StdAfx.h"
#include "PositioningManager.h"

#define POSITION_MANAGER_QUEUE_SIZE 10000

CPositioningManager::CPositioningManager(void) : CThreadWithQueue("PositioningManager", POSITION_MANAGER_QUEUE_SIZE)
{
}

CPositioningManager::~CPositioningManager(void)
{
}


bool CPositioningManager::Init()
{
	// Create Sensors & Sensors Container

	return true;
}