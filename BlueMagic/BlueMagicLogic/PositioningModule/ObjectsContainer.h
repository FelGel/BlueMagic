#pragma once

#include <vector>

template <class Object> class CObjectsContainer
{
public:
	CObjectsContainer()
	{
	}

	~CObjectsContainer()
	{
		int Count = m_Objects.size();
		for (int i = 0; i < Count; ++i)
		{
			m_Objects[i]->Close();
			delete m_Objects[i];
		}
	}

	bool CreateObjects(
		const char* Name, 
		const char* ConfigSection, 
		const char* Prefix, 
		int MaxNumber)
	{
		std::vector<std::string> ObjectSections = GetConfigSectionsArray(ConfigSection,Prefix,MaxNumber);    
		for (int i = 0; (unsigned)i < ObjectSections.size(); ++i) 
		{
			std::string ObjectSection = ObjectSections[i];
			Object* Object = CreateObject(ObjectSection.c_str());
			if (Object)
				m_Objects.push_back(Object);
			else
				LogEvent(LE_ERROR, "CObjectContainer::CreateObjects: error creating %s[%s]", 
				Name, ObjectSection.c_str());
		}
		if (m_Objects.size() == 0)
		{
			LogEvent(LE_WARNING, "CObjectContainer::CreateObjects: No %ss were created", Name);
			return false;
		}

		LogEvent(LE_INFOHIGH, "CObjectContainer::CreateObjects: Created %d %ss", m_Objects.size(), Name);
		return true;
	}

	void RemoveObjects()
	{
		for(int i=0; (unsigned int)i<m_Objects.size(); i++)
			delete m_Objects[i];
		m_Objects.clear();
	}

	void DisplayCcLinksData()
	{
		for(int i=0; (unsigned int)i<m_Objects.size(); i++)
			m_Objects[i]->DisplayCcLinksData();
	}

protected:
	virtual Object* CreateObject(const char* ConfigSection) = 0;

private:
	std::vector<Object*> m_Objects;
};
