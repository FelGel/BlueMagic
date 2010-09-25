#pragma once

#pragma warning(disable:4239 4996)
#include "geometry.h"
#pragma warning(default:4239 4996)

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
