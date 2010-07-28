#pragma once
#include "ObjectsContainer.h"
#include "SensorController.h"
#include "ISensorEvents.h"

class CSensorControllersContainer :
	public CObjectsContainer<CSensorController>
{
public:
	CSensorControllersContainer(void);
	~CSensorControllersContainer(void);

	bool CreateObjects(
		const char* ConfigSection, ISensorEvents* Handler);

	void RemoveObjects();

private:
	// Implemented CSensorControllersContainer method
	virtual CSensorController* CreateObject(const char* ConfigSection);

	ISensorEvents* m_Handler;
};
