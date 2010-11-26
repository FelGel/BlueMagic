#include "StdAfx.h"
#include "EstablishmentTopology.h"
#include "Common/Config.h"
#include "Common/LogEvent.h"

static const char* CONFIG_SECTION = "EstablishmentTopology";
static const char* DepartmentsConfigurationSection = "DepartmentsTopology";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CEstablishmentTopology::CEstablishmentTopology(void) : m_EstablishmentContour("Establishment")
{
}

CEstablishmentTopology::~CEstablishmentTopology(void)
{
}


bool CEstablishmentTopology::Init()
{
	m_EstablishmentID = GetConfigInt(CONFIG_SECTION, "EstablishmentID", -1);

	std::string EstablishmentContourFileName = GetConfigString(CONFIG_SECTION, "EstablishmentContourFileName", "");

	if (m_EstablishmentID == -1)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": EstablishmentID is not configured correctly!");
		return false;
	}

	if (EstablishmentContourFileName == "")
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed EstablishmentContour File Name not specified !");
		return false;
	}

	if (!m_EstablishmentContour.Init(EstablishmentContourFileName.c_str()))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Init Establishment via EstablishmentContour File (%s) !", EstablishmentContourFileName.c_str());
		return false;
	}

	if (!m_DepartmentsContainer.CreateObjects(DepartmentsConfigurationSection))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to Create & Init Department objects !");
		return false;
	}

	return true;
}

std::vector<SPosition> CEstablishmentTopology::GetEstablishmentCoordinates()
{
	return m_EstablishmentContour.GetDepartmentCoordinates();
}

bool CEstablishmentTopology::IsMeasurementInEstablishemnt(SPosition Position)
{
	return m_EstablishmentContour.IsMeasurementInDepartment(Position);
}

std::vector<SDepartmentInfo> CEstablishmentTopology::GetDepartmentsInfo()
{
	std::vector<SDepartmentInfo> DepartmentsInfo;

	for (unsigned int i = 0; i < m_DepartmentsContainer.GetNumberOfDepartments(); i++)
	{
		CDepartmentObject* DepartmentObject = m_DepartmentsContainer.GetDepartmentAt(i);
		DepartmentsInfo.push_back(DepartmentObject->GetDepartmentInfo());
	}

	return DepartmentsInfo;
}

void CEstablishmentTopology::Close()
{
	m_DepartmentsContainer.RemoveObjects();
}

std::vector<std::string> CEstablishmentTopology::GetDepartmentNamesUserCurrentlyIn(SPosition Position)
{
	std::vector<std::string> DepartmentsNames;

	for (unsigned int i = 0; i < m_DepartmentsContainer.GetNumberOfDepartments(); i++)
	{
		CDepartmentObject* DepartmentObject = m_DepartmentsContainer.GetDepartmentAt(i);
		if (DepartmentObject->IsMeasurementInDepartment(Position))
		{
			DepartmentsNames.push_back(DepartmentObject->GetDepartmentName());
		}
	}

	return DepartmentsNames;
}