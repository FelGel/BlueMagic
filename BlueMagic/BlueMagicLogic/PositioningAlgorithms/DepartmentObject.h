#pragma once

#include "geometry.h"
#include "PositionStructure.h"
#include <vector>

struct SDepartmentInfo
{
	std::vector<SPosition> DepartmentCoordinates;
	std::string DepartmentName;
};

class CDepartmentObject
{
public:
	CDepartmentObject(std::string DepartmentName);
	~CDepartmentObject(void);

	bool Init(std::string DepartmentFileName);
	void Close();

	SDepartmentInfo GetDepartmentInfo();

	std::vector<SPosition> GetDepartmentCoordinates();
	bool IsMeasurementInDepartment(SPosition Position);

	std::string GetDepartmentName() {return m_DepartmentName;}
	std::string GetDepartmentFileName() {return m_DepartmentFileName;}

private:
	std::string m_DepartmentName;
	std::string m_DepartmentFileName;
	CPolygon m_DepartmentContour;
};
