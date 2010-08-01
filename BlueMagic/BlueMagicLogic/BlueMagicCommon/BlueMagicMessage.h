#pragma once

#include "Common\Serializer.h"
#include "Common\DeSerializer.h"


enum EBlueMagicMessageType
{
	BlueMagicBlueToothMessageType = 0, 
	BlueMagicWorkstationMessageType = 100, 
	/*... up to 255*/
};

// BlueMagic Message Abstract Parent:
// -----------------------------
class CBlueMagicMessage
{
public:
	virtual                     ~CBlueMagicMessage() {}
	virtual bool				Serialize(ISerializer *Serializer) const = 0;
	virtual bool				DeSerialize(IDeSerializer *DeSerializer) = 0;
	virtual int                 MessageType() const = 0;
	virtual int					MessageLength() const = 0;
};
