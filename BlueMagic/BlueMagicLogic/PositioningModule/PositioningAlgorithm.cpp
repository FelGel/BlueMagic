#include "StdAfx.h"
#include "PositioningAlgorithm.h"

CPositioningAlgorithm::CPositioningAlgorithm(void)
{
}

CPositioningAlgorithm::~CPositioningAlgorithm(void)
{
}

void CPositioningAlgorithm::Advise(CEstablishmentTopology* EstablishmentTopology, IPositioningEvents *PositioningEventsHandler)
{
	m_EstablishmentTopology = EstablishmentTopology;
	m_PositioningEventsHandler = PositioningEventsHandler;
}

void CPositioningAlgorithm::OnScannedData(const SScannedData& ScannedData)
{
	// ToDo :
	// 1. process
	// 2. update local DB if needed
	// 3. issue a Positioning Event if needed via IPositioningEvents interface

	// TEMP: for TESTING
	SPosition Position(0,0);
	m_PositioningEventsHandler->OnPositioning(ScannedData.ScannedBDADDRESS, Position, 0, ScannedData.Time, m_EstablishmentTopology->GetEstablishmentID(), false);
}