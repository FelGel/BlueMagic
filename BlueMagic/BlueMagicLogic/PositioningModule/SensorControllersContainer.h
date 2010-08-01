#pragma once
#include "BlueMagicCommon\ObjectsContainer.h"
#include "SensorController.h"
#include "ISensorEvents.h"

class CSensorControllersContainer :
	public CObjectsContainer<CSensorController>
{
public:
	CSensorControllersContainer(void);

	bool CreateObjects(
		const char* ConfigSection, ISensorEvents* Handler);

	void RemoveObjects();

private:
	// Implemented CSensorControllersContainer method
	virtual CSensorController* CreateObject(const char* ConfigSection, int ObjectIndex);
	virtual void OnRemoveObject(CSensorController *RemovedObject);
	bool AreObjectParametersValid(int ObjectIndex, int SensorID, int ComPort, std::string BDADDRESS);

	ISensorEvents* m_Handler;
};
