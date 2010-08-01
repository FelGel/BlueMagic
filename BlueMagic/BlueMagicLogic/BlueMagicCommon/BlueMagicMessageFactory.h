#pragma once

#include <map>

////////////////////////////////////////////////////////////////////////////////////////////
// CBlueMagicMessage Factory:
////////////////////////////////////////////////////////////////////////////////////////////

template<typename MessageBaseClass> class IBlueMagicMessageCreator
{
public:
	virtual MessageBaseClass *CreateBlueMagicMessage() = 0;
};

template<typename MessageType, typename MessageClass, typename MessageBaseClass>
class CBlueMagicMessageCreator : public IBlueMagicMessageCreator<MessageBaseClass>
{
public:
	CBlueMagicMessageCreator(MessageType Type)
	{
		CBlueMagicMessageFactory<MessageType, MessageBaseClass>::GetBlueMagicMessageFactory()->RegisterBlueMagicMessageType(Type, this);
	}
	virtual MessageBaseClass *CreateBlueMagicMessage()
	{
		return new MessageClass;
	}
};

template<typename MessageType, typename MessageBaseClass> class CBlueMagicMessageFactory
{
public:
	// Static access function
	static CBlueMagicMessageFactory *GetBlueMagicMessageFactory();

	// Local functions:
	MessageBaseClass *CreateBlueMagicMessage(MessageType Type)
	{
		std::map<MessageType, IBlueMagicMessageCreator<MessageBaseClass> *>::const_iterator MapIterator;

		// Make sure the message being searched is in the map.
		MapIterator = m_TypeToMessageMap.find(Type) ;
		Assert(MapIterator != m_TypeToMessageMap.end());
		if (MapIterator == m_TypeToMessageMap.end())
			return NULL;

		return ((*MapIterator).second)->CreateBlueMagicMessage();
	}

	bool RegisterBlueMagicMessageType(MessageType Type, IBlueMagicMessageCreator<MessageBaseClass> *BlueMagicMessageCreator)
	{
		std::pair<MessageType, IBlueMagicMessageCreator<MessageBaseClass> *> MessagePair(Type, BlueMagicMessageCreator);

		// Make sure the message being registered is not already in the map.
		//Assert(m_TypeToMessageMap.find(MessageType) == m_TypeToMessageMap.end());
		if (m_TypeToMessageMap.find(Type) != m_TypeToMessageMap.end())
			return false;

		m_TypeToMessageMap.insert(MessagePair);

		return true;
	}

private:
	// Local Map Member:
	std::map<MessageType, IBlueMagicMessageCreator<MessageBaseClass>  *> m_TypeToMessageMap;
};

template<typename MessageType, typename MessageBaseClass> 
CBlueMagicMessageFactory<MessageType, MessageBaseClass> * CBlueMagicMessageFactory<MessageType, MessageBaseClass>::GetBlueMagicMessageFactory()
{
	static CBlueMagicMessageFactory<MessageType, MessageBaseClass> BlueMagicMessageFactorySingleton;

	return &BlueMagicMessageFactorySingleton;
}

#define RegisterBlueMagicMessage(Type, MessageType, MessageClass, MessageBaseClass)											\
	static CBlueMagicMessageCreator<MessageType, MessageClass, MessageBaseClass>  MessageClass ## CreatorInstance(Type);
