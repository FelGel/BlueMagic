#include "StdAfx.h"
#include "DepartmentObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDepartmentObject::CDepartmentObject(std::string DepartmentName) : m_DepartmentName(DepartmentName) {}
CDepartmentObject::~CDepartmentObject(void) {}

bool CDepartmentObject::Init(std::string DepartmentFileName)
{
	m_DepartmentFileName = DepartmentFileName;

	if (!m_DepartmentContour.LoadXYZ(DepartmentFileName.c_str()))
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Failed to read Department File (%s) !", DepartmentFileName.c_str());
		return false;
	}

	if (m_DepartmentContour.GetSize() < 3)
	{
		LogEvent(LE_ERROR, __FUNCTION__ ": Department File (%s) contains only %d Vertices!", 
			DepartmentFileName.c_str(), m_DepartmentContour.GetSize());
		return false;
	}

	for (int i = 0; i < m_DepartmentContour.GetSize(); i++)
	{
		if (m_DepartmentContour[i].z != 0)
		{
			// because PointIn function checks polygon in 3d,
			// z must also be accurate, and must always be 0.
			LogEvent(LE_WARNING, __FUNCTION__ ": Z values other than 0 are not supported! Setting Z=0.");
			m_DepartmentContour[i].z = 0;
		}
	}

	LogEvent(LE_INFOHIGH, __FUNCTION__ ": %s Initialized (Topology)", m_DepartmentName.c_str());

	return true;
}

void CDepartmentObject::Close()
{
	// Do Nothing..
}

std::vector<SPosition> CDepartmentObject::GetEstablishmentCoordinates()
{
	std::vector<SPosition> DepartmentCoordinates;

	for (int i = 0; i < m_DepartmentContour.GetSize(); i++)
		DepartmentCoordinates.push_back(SPosition(m_DepartmentContour[i].x, m_DepartmentContour[i].y));

	return DepartmentCoordinates;
}

bool CDepartmentObject::IsMeasurementInEstablishemnt(SPosition Position)
{
	C3Point Point(Position.x, Position.y, 0);
	return (m_DepartmentContour.PointIn(Point) == TRUE) ? true : false;
}