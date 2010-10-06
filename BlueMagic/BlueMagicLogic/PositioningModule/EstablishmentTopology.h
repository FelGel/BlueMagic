#pragma once

#include "geometry.h"

class CEstablishmentTopology
{
public:
	CEstablishmentTopology(void);
	~CEstablishmentTopology(void);

	bool Init();

	int GetEstablishmentID() {return m_EstablishmentID;}

private:
	//members:
	int m_EstablishmentID;
	CPolygon m_EstablishmentContour;
	CPolygon m_SensorsArrayContour;
};
