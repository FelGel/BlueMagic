#pragma once

#include <string>

class CCuidGenerator
{
public:
	CCuidGenerator(void);
	~CCuidGenerator(void);

	static std::string ConvertToCuid(std::string BDADDRESS);
};
