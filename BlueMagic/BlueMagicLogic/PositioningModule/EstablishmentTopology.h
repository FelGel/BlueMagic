#pragma once

#include "geometry.h"
#include "PositioningAlgorithms/PositionStructure.h"
#include <vector>

class CEstablishmentTopology
{
public:
	CEstablishmentTopology(void);
	~CEstablishmentTopology(void);

	bool Init();

	int GetEstablishmentID() {return m_EstablishmentID;}
	std::vector<SPosition> GetEstablishmentCoordinates();

private:
	//members:
	int m_EstablishmentID;
	CPolygon m_EstablishmentContour;
	CPolygon m_SensorsArrayContour;
};
