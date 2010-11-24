#include "StdAfx.h"
#include "DepartmentsContainer.h"

#include "..\Common\Config.h"
#include "..\common\Utils.h"

const char* DepartmentObjectPrefix	= "Department";
const int	MaxDepartments			= 10000;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CDepartmentsContainer::CDepartmentsContainer(void)
{
}

bool CDepartmentsContainer::CreateObjects(const char* ConfigSection)
{
	return CObjectsContainer<CDepartmentObject>::CreateObjects(
		DepartmentObjectPrefix, 
		ConfigSection, 
		DepartmentObjectPrefix, 
		MaxDepartments);
}

void CDepartmentsContainer::RemoveObjects()
{
	CObjectsContainer<CDepartmentObject>::RemoveObjects();
}

void CDepartmentsContainer::OnRemoveObject(CDepartmentObject * /*RemovedObject*/)
{
	// Do nothing.
}

//#include "DepartmentObjectGenericFactory.h"

// Implemented CTxRxContainer method
CDepartmentObject* CDepartmentsContainer::CreateObject(const char* ConfigSection, int ObjectIndex)
{
	std::string DepartmentName = GetStringValue(ConfigSection, "DepartmentName=", "");
	std::string DepartmentTopologyFileName = GetStringValue(ConfigSection, "TopologyFileName=", "");

	if (!AreObjectParametersValid(ObjectIndex, DepartmentName, DepartmentTopologyFileName)
		|| IsDepartmentNameAlreadyExists(DepartmentName, DepartmentTopologyFileName))
		return NULL;

	CDepartmentObject* DepartmentObject = NULL;
	DepartmentObject = new CDepartmentObject(DepartmentName);
	if(DepartmentObject == NULL)
		return NULL;

	if (DepartmentObject->Init(DepartmentTopologyFileName))
	{
		LogEvent(LE_INFOHIGH, 
			"CDepartmentsContainer::CreateObject: Created DepartmentObject[%d]", ObjectIndex);
		return DepartmentObject;
	}

	// ToDo: 
	// Following Init, make sure departments do NOT OVERLAP !!

	LogEvent(LE_ERROR, 
		"CDepartmentsContainer::CreateObject: error in init DepartmentObject[%d]", ObjectIndex);
	delete DepartmentObject;
	return NULL;
}


bool CDepartmentsContainer::AreObjectParametersValid(int ObjectIndex, std::string DepartmentName, std::string DepartmentTopologyFileName)
{
	bool IsValid = true;

	if (DepartmentName == "")
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Department %d has no Name !", ObjectIndex);
		IsValid = false;
	}

	if (DepartmentTopologyFileName == "")
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Department %d has no TopologyFileName !", ObjectIndex);
		IsValid = false;
	}

	return IsValid;
}

bool CDepartmentsContainer::IsDepartmentNameAlreadyExists(std::string DepartmentName, std::string DepartmentTopologyFileName)
{
	for (unsigned int i = 0; i < GetNumberOfSensorControllers(); i++)
	{
		CDepartmentObject*	DepartmentObject = GetSensorControllerAt(i);
		if (DepartmentObject->GetDepartmentName() == DepartmentName)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Department Name %s already exists! Did you accidentally entered same department twice?", 
				DepartmentName.c_str());	
			return true;
		}

		if (DepartmentObject->GetDepartmentFileName() == DepartmentTopologyFileName)
		{
			LogEvent(LE_ERROR, __FUNCTION__ ": Department File Name %s is already in use by other department (%s) besides %s ! Did you accidentally entered same department twice?", 
				DepartmentTopologyFileName.c_str(), DepartmentObject->GetDepartmentName().c_str(), DepartmentName.c_str());	
			return true;
		}
	}

	return false;
}