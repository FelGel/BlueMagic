#include "StdAfx.h"
#include "CuidGenerator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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