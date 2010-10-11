#pragma once

#include "..\common\threadwithqueue.h"
#include "ISensorEvents.h"
#include "SensorControllersContainer.h"
#include "EstablishmentTopology.h"
#include "PositioningAlgorithm.h"
#include "DialogMessages.h"

class CPositioningManager :	public CThreadWithQueue, public ISensorEvents, public IPositioningEvents
{
public:
	CPositioningManager(void);
	~CPositioningManager(void);

	// SensorEvents
	virtual void OnErrorInTopology(UCHAR SensorId);
	virtual void OnSensorInfo(int SensorId, SSensorInfo SensorInfo);
	virtual void OnIncomingScannedData(int SensorId, SScannedData ScannedData);
	virtual void OnConnected(UCHAR SensorId);
	virtual void OnDisconnected(UCHAR SensorId);
	virtual void OnSensorInSystem(int SensorId, bool IsController, std::string BDADDRESS, std::vector<int> ChildrenSensorIDs);

	// Positioning Algorithm Events
	virtual void OnPositioning(std::string BDADDRESS, SPosition Position, double Accuracy, DWORD TimeStamp, int StoreID, bool IsInStore);

	// General Public functions
	void Advise(IPositioningInterface *PositioningInterfaceHandler) {m_PositioningInterfaceHandler = PositioningInterfaceHandler;}
	void Advise(IDialogMessagesInterface *DialogMessagesInterfaceHandler) {m_DialogMessagesInterfaceHandler = DialogMessagesInterfaceHandler;}
	
	bool Init();
	virtual void OnThreadClose();

	// Debugging & Monitoring
	void CreateScanFile(const int SensorId);
	void CloseAllScanFiles();
	void UpdateScanFile(const int &SensorId, const SScannedData& ScannedData);
	void UpdateDialog(const int &SensorId, const SScannedData& ScannedData);

private:
	void HandleDataReceived(const int &SensorId, const SScannedData& ScannedData);
	void HandleNewSensorInSystem(const int &SensorId, const bool &IsController, const std::string &BDADDRESS, const std::vector<int> &ChildrenSensorIDs);

private:
	CSensorControllersContainer m_SensorControllersContainer;
	CEstablishmentTopology m_EstablishmentTopology;//CEstablishmentTopologysContainer m_Establishments; 
	CPositioningAlgorithm m_PositioningAlgorithm;
	IPositioningInterface *m_PositioningInterfaceHandler;

	/* TEMP -> Write to File*/
	std::map<int /*SensorID*/, CStdioFile*> m_ScanFiles; // SensorId to ScanFile
	IDialogMessagesInterface *m_DialogMessagesInterfaceHandler;
};
