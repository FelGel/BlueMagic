#include "StdAfx.h"
#include "SensorController.h"

#define SENSOR_CONTROLLER_QUEUE_SIZE 10000

CSensorController::CSensorController(void) : CThreadWithQueue("SensorController", SENSOR_CONTROLLER_QUEUE_SIZE)
{
}

CSensorController::~CSensorController(void)
{
}

bool CSensorController::Init(ISensorEvents *Handler)
{
	m_EventsHandler = Handler;
	return true;
}

void CSensorController::Close()
{

}