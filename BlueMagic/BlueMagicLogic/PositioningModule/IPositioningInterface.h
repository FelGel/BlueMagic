#pragma once
#include <string>
#include "PositioningAlgorithms/PositionStructure.h"

// Already defined in PositioningBasicAlgorithm.h
// struct SPosition
// {
// 	struct SPosition(double _x, double _y): x(_x), y(_y) {}
// 	double x;
// 	double y;
// };

class IPositioningInterface
{
public:
	virtual void OnPosition(std::string CUID, SPosition Position, double Accuracy, DWORD TimeStamp, int StoreID, bool IsInStore, std::vector<std::string> DepartmentNamesUserCurrentlyIn) = 0;
};