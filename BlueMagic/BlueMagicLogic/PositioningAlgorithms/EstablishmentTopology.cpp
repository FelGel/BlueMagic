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
	return m_EstablishmentContour.GetEstablishmentCoordinates();
}

bool CEstablishmentTopology::IsMeasurementInEstablishemnt(SPosition Position)
{
	return m_EstablishmentContour.IsMeasurementInEstablishemnt(Position);
}