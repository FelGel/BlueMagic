#pragma once
#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"

class CSensorController :
	public CThreadWithQueue
{
public:
	CSensorController(void);
	~CSensorController(void);

	bool Init(ISensorEvents *Handler);

	void Close(); // called right before it is deleted

	ISensorEvents *m_EventsHandler;
};
