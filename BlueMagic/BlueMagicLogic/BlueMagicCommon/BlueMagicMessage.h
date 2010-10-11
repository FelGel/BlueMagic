#pragma once

#include "Common\Serializer.h"
#include "Common\DeSerializer.h"

#define BLUEMAGIC_VERSION	1

#define MIN_VALID_SENSORID  1
#define MAX_VALID_SENSORID  255


enum EBlueMagicMessageType
{
	BlueMagicBTBIncomingMessageType = 0x30, 
	BlueMagicBTBOutgoingMessageType = 0x36, 
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
