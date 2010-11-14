#include "StdAfx.h"
#include "EstablishmentTopology.h"
#include "Common/Config.h"
#include "Common/LogEvent.h"

static const char* CONFIG_SECTION = "EstablishmentTopology";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CEstablishmentTopology::CEstablishmentTopology(void)
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


	if (!m_EstablishmentContour.LoadXYZ(EstablishmentContourFileName.c_str()))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to read EstablishmentContour File (%s) !", EstablishmentContourFileName.c_str());
		return false;
	}

	for (int i = 0; i < m_EstablishmentContour.GetSize(); i++)
	{
		if (m_EstablishmentContour[i].z != 0)
		{
			// because PointIn function checks polygon in 3d,
			// z must also be accurate, and must always be 0.
			LogEvent(LE_WARNING, __FUNCTION__ ": Z values other than 0 are not supported! Setting Z=0.");
			m_EstablishmentContour[i].z = 0;
		}
	}

	return true;
}

std::vector<SPosition> CEstablishmentTopology::GetEstablishmentCoordinates()
{
	std::vector<SPosition> EstablishmentCoordinates;

	for (int i = 0; i < m_EstablishmentContour.GetSize(); i++)
		EstablishmentCoordinates.push_back(SPosition(m_EstablishmentContour[i].x, m_EstablishmentContour[i].y));

	return EstablishmentCoordinates;
}

bool CEstablishmentTopology::IsMeasurementInEstablishemnt(SPosition Position)
{
	C3Point Point(Position.x, Position.y, 0);
	return (m_EstablishmentContour.PointIn(Point) == TRUE) ? true : false;
}