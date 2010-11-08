#pragma once

#include "geometry.h"
#include "PositioningAlgorithms.h"
#include "PositionStructure.h"
#include <vector>

class POSITIONINGALGORITHMS_API CEstablishmentTopology
{
public:
	CEstablishmentTopology(void);
	~CEstablishmentTopology(void);

	bool Init();

	int GetEstablishmentID() {return m_EstablishmentID;}
	std::vector<SPosition> GetEstablishmentCoordinates();

	bool IsMeasurementInEstablishemnt(SPosition Position);

private:
	//members:
	int m_EstablishmentID;
	CPolygon m_EstablishmentContour;
};
