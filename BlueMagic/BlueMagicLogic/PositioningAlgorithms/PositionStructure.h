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

#define INVALID_MEASUREMENT -99999999
const SPosition InvalidPosition = SPosition(INVALID_MEASUREMENT, INVALID_MEASUREMENT);