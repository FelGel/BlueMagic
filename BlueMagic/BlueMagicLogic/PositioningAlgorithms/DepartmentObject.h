#pragma once

#include "geometry.h"
#include "PositionStructure.h"
#include <vector>

class CDepartmentObject
{
public:
	CDepartmentObject(std::string DepartmentName);
	~CDepartmentObject(void);

	bool Init(std::string DepartmentFileName);
	void Close();

	std::vector<SPosition> GetEstablishmentCoordinates();
	bool IsMeasurementInEstablishemnt(SPosition Position);

	std::string GetDepartmentName() {return m_DepartmentName;}
	std::string GetDepartmentFileName() {return m_DepartmentFileName;}

private:
	std::string m_DepartmentName;
	std::string m_DepartmentFileName;
	CPolygon m_DepartmentContour;
};
