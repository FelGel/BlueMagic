#pragma once

#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"

class CPositioningManager :	public CThreadWithQueue, public ISensorEvents
{
public:
	CPositioningManager(void);
	~CPositioningManager(void);

	bool Init();
};
