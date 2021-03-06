#pragma once

#include "PositioningAlgorithms.h"
#include "PositionStructure.h"
#include "DepartmentObject.h"
#include "DepartmentsContainer.h"
#include <vector>

class POSITIONINGALGORITHMS_API CEstablishmentTopology
{
public:
	CEstablishmentTopology(void);
	~CEstablishmentTopology(void);

	bool Init();
	void Close();

	int GetEstablishmentID() {return m_EstablishmentID;}
	std::vector<SPosition> GetEstablishmentCoordinates();

	bool IsMeasurementInEstablishemnt(SPosition Position);
	std::vector<std::string> GetDepartmentNamesUserCurrentlyIn(SPosition Position);

	std::vector<SDepartmentInfo> GetDepartmentsInfo();

private:
	//members:
	int m_EstablishmentID;
	CDepartmentObject m_EstablishmentContour;
	CDepartmentsContainer m_DepartmentsContainer;
};
