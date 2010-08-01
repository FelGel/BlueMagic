#pragma once

class ISensorCommands
{
public:
	virtual void GetInfo() = 0;
	virtual void GetData() = 0;
	virtual void DefineTopology(/*......*/) = 0;
};