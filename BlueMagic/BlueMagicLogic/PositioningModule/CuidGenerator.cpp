#include "StdAfx.h"
#include "CuidGenerator.h"

CCuidGenerator::CCuidGenerator(void)
{
}

CCuidGenerator::~CCuidGenerator(void)
{
}


/*static*/ std::string CCuidGenerator::ConvertToCuid(std::string BDADDRESS)
{
	return BDADDRESS; // currently NOT converting.
	// in the future, might connect to a central CUID server for conversions!

	// FEAR: duplicated CUIDs for one BDADDRESS.
}