#include "StdAfx.h"
#include "SensorControllersContainer.h"
#include "..\Common\Config.h"

CSensorControllersContainer::CSensorControllersContainer() : m_Handler(NULL)
{
}

bool CSensorControllersContainer::CreateObjects(
									 const char* ConfigSection, ISensorEvents* Handler)
{
	const char* SensorControllerPrefix = "SensorController";
	const int MaxReceivers       = 100;

	m_Handler = Handler;

	return CObjectsContainer<CSensorController>::CreateObjects(
		"SensorController", 
		ConfigSection, 
		SensorControllerPrefix, 
		MaxReceivers);
}

void CSensorControllersContainer::RemoveObjects()
{
	CObjectsContainer<CSensorController>::RemoveObjects();
}

//#include "SensorControllerGenericFactory.h"

// Implemented CTxRxContainer method
CSensorController* CSensorControllersContainer::CreateObject(const char* ConfigSection)
{
	std::string ReceiverType = GetConfigString(ConfigSection, "ReceiverType", "");
	int SensorID = GetConfigInt(ConfigSection, "SensorID", 0);//GetConfigSectionId(ConfigSection);   

	CSensorController* SensorController = NULL;
	SensorController = new CSensorController();//CREATE_CC_RECEIVER(ReceiverType, ConfigSection, ReceiverId);
	if(SensorController == NULL)
		return NULL;

	if (m_Handler != NULL && SensorController->Init(m_Handler))
	{
		LogEvent(LE_INFOHIGH, 
			"CCcRxContainer::CreateObject: Created [%s]", ReceiverType.c_str());
		return SensorController;
	}

	LogEvent(LE_ERROR, 
		"CCcRxContainer::CreateObject: error in init [%s]", ReceiverType.c_str());
	delete SensorController;
	return NULL;
}
