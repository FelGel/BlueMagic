#pragma once

#include "PositioningAlgorithms.h"

struct POSITIONINGALGORITHMS_API SPosition
{
	SPosition() : x(0), y(0) {}
	SPosition(double _x, double _y) : x(_x), y(_y) {}

	BOOL operator==(const SPosition& other) {return (this->x == other.x && this->y == other.y);}

	double x;
	double y;
};

#define IRRELEVANT_RSSI -1000

struct POSITIONINGALGORITHMS_API SMeasurement
{
	SMeasurement() : m_RSSI(IRRELEVANT_RSSI), m_TickCount(0) {}

	int		m_RSSI;
	DWORD	m_TickCount;
};

#define INVALID_MEASUREMENT -99999999
const SPosition InvalidPosition = SPosition(INVALID_MEASUREMENT, INVALID_MEASUREMENT);