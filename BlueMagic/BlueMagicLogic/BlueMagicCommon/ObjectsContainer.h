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
		// Since we use INI infrastructure instead of XML:
		//////////////////////////////////////////////////////////////////////////
		// std::vector<std::string> ObjectSections = GetConfigSectionsArray(ConfigSection,Prefix,MaxNumber);    

		std::vector<ConfigListItem> ObjectSections;
		GetListSection(ConfigSection, Prefix, ObjectSections);
		//////////////////////////////////////////////////////////////////////////

		if (ObjectSections.size() == 0)
		{
			LogEvent(LE_ERROR, "Failed to read objects from Configuration !");
			return false;
		}

		bool ObjectsCreatedOk = true;
		for (int i = 0; (unsigned)i < min(ObjectSections.size(), (unsigned)MaxNumber); ++i) 
		{
			std::string ObjectSection = ObjectSections[i].ItemValue;
			Object* ObjectInstance = CreateObject(ObjectSection.c_str(), ObjectSections[i].ItemId);
			if (ObjectInstance)
				m_Objects.push_back(ObjectInstance);
			else
			{
				LogEvent(LE_ERROR, "CObjectContainer::CreateObjects: error creating %s[%s]", 
				Name, ObjectSection.c_str());
				ObjectsCreatedOk = false;
			}
		}

		if (m_Objects.size() == 0)
		{
			LogEvent(LE_WARNING, "CObjectContainer::CreateObjects: No %ss were created", Name);
			return false;
		}

		LogEvent(LE_INFOHIGH, "CObjectContainer::CreateObjects: Created %d %ss", m_Objects.size(), Name);
		return ObjectsCreatedOk; //true;
	}

	void RemoveObjects()
	{
		for(int i=0; (unsigned int)i<m_Objects.size(); i++)
		{
			OnRemoveObject(m_Objects[i]);
			delete m_Objects[i];
		}
		m_Objects.clear();
	}

protected:
	virtual Object* CreateObject(const char* ConfigSection, int ObjectIndex) = 0;
	virtual void OnRemoveObject(Object *RemovedObject) = 0;

	DWORD	GetNumberOfObjects()	{return m_Objects.size();}
	Object* GetObjectAt(int index)	{return m_Objects[index];}

private:
	std::vector<Object*> m_Objects;
};
