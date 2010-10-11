#include "StdAfx.h"
#include "SensorControllersContainer.h"
#include "..\Common\Config.h"
#include "..\common\Utils.h"

const char* SensorControllerPrefix	= "SensorController";
const int	MaxReceivers			= 100;

#define MIN_SENSORID				1
#define MAX_SENSORID				255
#define MIN_COMPORT					1
#define MAX_COMPORT					255
#define BDADDRESS_LENGTH_IN_CHARS	12  // nibles \ chars

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSensorControllersContainer::CSensorControllersContainer() : m_Handler(NULL)
{
}

bool CSensorControllersContainer::CreateObjects(
									 const char* ConfigSection, ISensorEvents* Handler)
{
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

void CSensorControllersContainer::OnRemoveObject(CSensorController *RemovedObject)
{
	RemovedObject->Close();
}

//#include "SensorControllerGenericFactory.h"

// Implemented CTxRxContainer method
CSensorController* CSensorControllersContainer::CreateObject(const char* ConfigSection, int ObjectIndex)
{
	int SensorID = GetIntValue(ConfigSection, "SensorID=", -1);//GetConfigSectionId(ConfigSection);   
	int ComPort  = GetIntValue(ConfigSection, "ComPort=", -1);
	std::string BDADDRESS = GetStringValue(ConfigSection, "BDADDRESS=", "");
	
	std::string ChildrenStr = GetStringValue(ConfigSection, "Children=", "");

	if (ChildrenStr == "")
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Empty Children list (Index %d SensorID %d)! If this is intentional, enter the keyword 'None' instead!", 
			ObjectIndex, SensorID);
		return NULL;
	}
	
	std::vector<int> ChildrenSensorIDs;
	if (ChildrenStr != "None")
	{
		ParseIntVectorString(ChildrenStr.c_str(), ChildrenSensorIDs);
		if (ChildrenSensorIDs.size() == 0)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Empty or Invalid Children list (Index %d SensorID %d)! If this is intentional, enter the keyword 'None' instead!", 
				ObjectIndex, SensorID);
			return NULL;
		}
	}
	
	if (!AreObjectParametersValid(ObjectIndex, SensorID, ComPort, BDADDRESS))
		return NULL;

	CSensorController* SensorController = NULL;
	SensorController = new CSensorController(SensorID, ComPort, BDADDRESS, ChildrenSensorIDs);
	if(SensorController == NULL)
		return NULL;

	if (m_Handler != NULL && SensorController->Init(m_Handler))
	{
		LogEvent(LE_INFOHIGH, 
			"CSensorControllersContainer::CreateObject: Created SensorController[%d]", ObjectIndex);
		return SensorController;
	}

	LogEvent(LE_ERROR, 
		"CSensorControllersContainer::CreateObject: error in init SensorController[%d]", ObjectIndex);
	delete SensorController;
	return NULL;
}


bool CSensorControllersContainer::AreObjectParametersValid(int ObjectIndex, int SensorID, int ComPort, std::string BDADDRESS)
{
	bool IsValid = true;

	if (SensorID < MIN_SENSORID || SensorID > MAX_SENSORID)
	{
		LogEvent(LE_ERROR, "SensorController #%d is mis-configured! SensorID=%d while allowed values are: [%d,%d]",
			ObjectIndex, SensorID, MIN_SENSORID, MAX_SENSORID);
		IsValid = false;
	}

	if (ComPort < MIN_COMPORT || ComPort > MAX_COMPORT)
	{
		LogEvent(LE_ERROR, "SensorController #%d is mis-configured! ComPort=%d while allowed values are: [%d,%d]",
			ObjectIndex, ComPort, MIN_COMPORT, MAX_COMPORT);
		IsValid = false;
	}

	if (BDADDRESS.length() != BDADDRESS_LENGTH_IN_CHARS)
	{
		LogEvent(LE_ERROR, "SensorController #%d is mis-configured! BDADDRESS length is %d while should be %d",
			ObjectIndex, BDADDRESS.length(), BDADDRESS_LENGTH_IN_CHARS);
		IsValid = false;
	}

	return IsValid;
}
