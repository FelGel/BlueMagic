#pragma once

#include "BlueMagicCommon\ObjectsContainer.h"
#include "DepartmentObject.h"

class CDepartmentsContainer :
	public CObjectsContainer<CDepartmentObject>
{
public:
	CDepartmentsContainer(void);

	bool CreateObjects(const char* ConfigSection);

	void RemoveObjects();

	DWORD				GetNumberOfSensorControllers()		{return GetNumberOfObjects();}
	CDepartmentObject*	GetSensorControllerAt(int index)	{return (CDepartmentObject*)GetObjectAt(index);}

private:
	// Implemented CSensorControllersContainer method
	virtual CDepartmentObject* CreateObject(const char* ConfigSection, int ObjectIndex);
	virtual void OnRemoveObject(CDepartmentObject *RemovedObject);
	bool AreObjectParametersValid(int ObjectIndex, std::string DepartmentName, std::string DepartmentTopologyFileName);
	bool IsDepartmentNameAlreadyExists(std::string DepartmentName, std::string DepartmentTopologyFileName);
};
