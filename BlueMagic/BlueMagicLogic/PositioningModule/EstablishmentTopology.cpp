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
	std::string SensorsArrayContourFileName = GetConfigString(CONFIG_SECTION, "SensorsArrayContourFileName", "");

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

	if (SensorsArrayContourFileName == "")
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed SensorsArrayContour File Name not specified !");
		return false;
	}

	if (EstablishmentContourFileName == SensorsArrayContourFileName)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Same file name was accidentally used for both EstablishmentContourFileName & SensorsArrayContourFileName (%s)!", EstablishmentContourFileName);
		return false;
	}

	if (!m_EstablishmentContour.LoadXYZ(EstablishmentContourFileName.c_str()))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to read EstablishmentContour File (%s) !", EstablishmentContourFileName.c_str());
		return false;
	}

	if (!m_SensorsArrayContour.LoadXYZ(SensorsArrayContourFileName.c_str()))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to read SensorsArrayContour File (%s) !", SensorsArrayContourFileName.c_str());
		return false;
	}

	return true;
}