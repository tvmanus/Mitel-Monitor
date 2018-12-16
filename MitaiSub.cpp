

#include "common.h"
#include "MitaiSub.h"

extern NotificationQueue NQ_MiTAI;
/* TM_Address controls individual monitors */

TM_Address::TM_Address( string dn, TM_PBX* pPBX)
{
	m_strDN = dn;
	m_pSwitch = pPBX;
	m_hMonitor = NULL;
	m_dwState = 0;

}


TM_Address::~TM_Address()
{
	StopMonitor();

}

DWORD TM_Address::SetMonitor()
{
	int iRet = SXERR_FAIL;

	hMonitorObject hMonTmp;

	
	if(m_pSwitch == NULL)
		return RESULT_FAIL;

	//We want to see MonitorSetEvent and determine initial state of the device
	//Register callback for all devices, but SetMonitorEvent only

	iRet = SXSetCallback(SX_PBX_HANDLE, m_pSwitch->m_hPBX,
						SX_CALLBACK_PROC, TM_Address::CallbackEventFromPBX,
						SX_CALLBACK_DATA_PTR, this,
						SX_EVENT, MonitorSetEvent,
						NULL);
	if(iRet != SX_OK)
	{
		Application::instance().logger().information("Failed to set the callback function for the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));
		
		return RESULT_FAIL;; 
	}

	iRet = SXActivateCallbacks(SX_PBX_HANDLE, m_pSwitch->m_hPBX, NULL );

	if(iRet != SX_OK)
	{
		Application::instance().logger().information("Address:Failed to active the callback function for the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));

		return RESULT_FAIL;
	}



	iRet = SXMonitor ( &m_hMonitor,  
					   SX_PBX_HANDLE, m_pSwitch->m_hPBX, 
					   SX_TRUNK_NUMBER,	atoi(m_strDN.c_str()), 
					   NULL ); //Check with API reference for correct parameters
	
	//samples for trunks and DNs
	//SX_TRUNK_NUMBER,	atoi(m_strDN.c_str()),
	//SX_DN,	m_strDN.c_str(),

	if(iRet == SX_OK)

	{
		//Need this only for Multi Call groups and Key system groups.
		if ( SX_IsMultiCallGroup (m_hMonitor) || SX_IsKeyline (m_hMonitor))
		{

			hMonTmp = m_hMonitor;
			iRet = SXMonitor (	&m_hMonitor,
								SX_MONITOR_HANDLE, m_hMonitor,
								SX_GROUP_MEMBER, 1,
								NULL);
			if (iRet != SX_OK)
			{
				Application::instance().logger().information("Address:Failed to monitor the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));

				return RESULT_FAIL;
			}

			iRet = SXStopMonitor(hMonTmp,NULL);
		}

		Application::instance().logger().information("Address:Monitor the line(" + m_strDN + ") successfully!");

	}
	else
	{
		Application::instance().logger().information("Address:Failed to monitor the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));

		return RESULT_FAIL;

	}

	//Setting callback

	iRet = SXSetCallback(SX_MONITOR_HANDLE, m_hMonitor,
						SX_CALLBACK_PROC, TM_Address::CallbackEventFromPBX,
						SX_CALLBACK_DATA_PTR, this,
						NULL);

	if(iRet != SX_OK)
	{
		Application::instance().logger().information("Failed to set the callback function for the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));
		
		return RESULT_FAIL;; 
	}

	//Activate callback for line events

	//iRet = SXActivateCallbacks(SX_PBX_HANDLE, m_pSwitch->m_hPBX, NULL );

	//if(iRet != SX_OK)
	//{
	//	printf("Address:Failed to active the callback function for the line(%s). Error Code: %s \r\n\r\n",m_strDN.c_str(), SX_ErrorText(iRet));

	//	return RESULT_FAIL;
	//}

	//Now we can clear global callback for MonitorSetEvent
	iRet = SXClearCallback(SX_PBX_HANDLE, m_pSwitch->m_hPBX,
						SX_CALLBACK_PROC, TM_Address::CallbackEventFromPBX,
						SX_CALLBACK_DATA_PTR, this,
						SX_EVENT, MonitorSetEvent,
						NULL);
	if(iRet != SX_OK)
	{
		Application::instance().logger().information("Failed to unset the callback function for the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));
		
		return RESULT_FAIL;; 
	}

	

	return RESULT_SUCCESS;

}


DWORD TM_Address::StopMonitor()
{
	int iRet = SXERR_FAIL;

	if(m_pSwitch == NULL)
		return RESULT_FAIL;	

	iRet = SXStopMonitor(m_hMonitor, NULL);
	m_hMonitor = NULL;


	if(iRet == SX_OK)
	{
		Application::instance().logger().information("Address:Stop Monitoring the line(" + m_strDN + ") successfully!");

	}
	else
	{
		Application::instance().logger().information("Address:Failed to stop monitoring the line(" + m_strDN + "). Error Code: " + std::string(SX_ErrorText(iRet)));

		return RESULT_FAIL;

	}
	return RESULT_SUCCESS;	
	

}

void TM_Address::CallbackEventFromPBX(SXEvent* pEvent, TM_Address* pAddress)
{
	std::string state_name;
	
	if(pEvent == NULL || pAddress == NULL)
		return;
	state_name = SXGetText (SX_STATE, SX_EventState(pEvent),NULL);

	Application::instance().logger().information("Event received...DN " + pAddress->m_strDN);//Check MiTAI for detailed trunk events description
	Application::instance().logger().information("Current state " + state_name);
	std::string strANI = SX_GetMainDn(SX_DeviceANI(SX_ThisDevice(pEvent)));
	pAddress->m_strANI = strANI;
	pAddress->m_dwState = SX_EventState(pEvent);
	pAddress->UpdateState();
	
}

string TM_Address::GetIPAddress()
{
	if(m_pSwitch != NULL)
		return m_pSwitch->GetIPAddress();
	else
		return "";
}

string TM_Address::GetDN()
{
		return m_strDN;
	
}

DWORD TM_Address::GetState()
{
	return m_dwState;
}

void TM_Address::UpdateState()

{
	std::string state_name;
//use std::string instead
	std::string msg = "<pbx ip_addr=\"" + GetIPAddress() + "\">";

	msg += "<line dn=\"";
	msg += m_strDN + "\" state_code=\"";
	//msg += _itoa_s(m_dwState,tmpI,10);
	msg += NumberFormatter::format(m_dwState);
	msg += "\" state_text=\"";
	msg += SXGetText (SX_STATE, m_dwState, NULL);
	msg += "\"/>";

	state_name = SXGetText (SX_STATE, m_dwState, NULL);

	
	msg += "</pbx>";

	Application::instance().logger().information("DN " + m_strDN +": " + state_name + "ANI:" + m_strANI);
	NQ_MiTAI.enqueueNotification(new MiTAINotification(msg));

/*	for (std::list<Socket*>::iterator os = TM_App::g_connections.begin();
                               os!=TM_App::g_connections.end(); 
                               os++) 
		{
			(*os)->SendLine(msg + "\r");
		}
*/	
}


/************TM_PBX**************/
TM_PBX::TM_PBX(string ipAddress, TM_Session* pSession)
{
	m_strIPAddress = ipAddress;

	m_hPBX = NULL;

	m_pSession = pSession;

	m_vpAddress.clear();


}

TM_PBX::~TM_PBX()
{
	ClosePBX();
	m_pSession = NULL;
	

}

DWORD TM_PBX::OpenPBX()
{
	int iRet = SXERR_FAIL;

	char charPBXIPAddress[40];
	strcpy(charPBXIPAddress, GetIPAddress().c_str());
	//GetIPAddress().copy(charPBXIPAddress,40);

	if(m_pSession == NULL)
		return RESULT_FAIL;
	iRet = SXOpenPBX(SX_PBX_HANDLE_PTR, &m_hPBX,
				SX_PBX_IPADDRESS, charPBXIPAddress,
				SX_COMMS_ERROR_PROC, &TM_PBX::CommErrorProc,			  
				SX_COMMS_ERROR_DATA_PTR, this,
				SX_KEEPALIVE_INTERVAL, DEF_KEEPALIVE_VALUE,
				SX_ADAPTER_IPADDRESS,Application::instance().config().getString("LocalIP").c_str(),
				NULL);


	if(iRet != SX_OK)
	{
		Application::instance().logger().information("PBX:Failed to Connect with 3300ICP(" + m_strIPAddress + "). Error Code:"+ std::string(SX_ErrorText(iRet)));
		
		return RESULT_FAIL;
	}

	Application::instance().logger().information("PBX:Connect with 3300ICP(%s) Successfully!" + m_strIPAddress);
	

	return RESULT_SUCCESS;

}

DWORD TM_PBX::ClosePBX()
{
	int iRet = SXERR_FAIL;
	
	iRet = StopAllMonitors();
	m_vpAddress.clear();	


	if(m_hPBX)
	{
		iRet = SXDeactivateCallbacks(SX_PBX_HANDLE, m_hPBX, NULL);		
		iRet = SXClosePBX(SX_PBX_HANDLE, m_hPBX,  
						  SX_CLOSE, NULL);	
		m_hPBX = NULL;
		if(iRet == SX_OK)
		{
			Application::instance().logger().information("PBX:Disconnect with 3300ICP(%s) successfully!" + m_strIPAddress);
			
		}
		else
		{
			Application::instance().logger().information("PBX:Failed to disconnect with 3300ICP(" + m_strIPAddress + "). Error Code: " + std::string(SX_ErrorText(iRet)));
			return RESULT_FAIL;
		}
				
			
	}

	return RESULT_SUCCESS;
	

}

DWORD TM_PBX::MonitorLine(string dn)
{
	Application::instance().logger().information("Line " + dn);
	TM_Address* m_pAddress = NULL;
	
	if(m_pAddress == NULL)
		m_pAddress = new TM_Address(dn, this);
	if(m_pAddress == NULL)
		return RESULT_FAIL;

	m_vpAddress.push_back(m_pAddress);
	return m_vpAddress.back()->SetMonitor();
	
}

void TM_PBX::MonitorLines(vector<string> dn)
{

	std::vector<string>::iterator	itDN = dn.begin();

	
		for (; itDN < dn.end(); itDN++)
		{
			MonitorLine(*itDN);
		}
}

DWORD TM_PBX::StopAllMonitors()
{
	//If we are going to reuse this, then we have to add more error handling
	//possible memory leack if we would not be able to delete objects
	
	std::vector<TM_Address*>::iterator	itAddr = m_vpAddress.begin();
	DWORD iRes = RESULT_SUCCESS;

	for (; itAddr < m_vpAddress.end(); itAddr++)
	{
			delete (*itAddr);
	}


	return iRes;
}

string TM_PBX::GetIPAddress()
{
	return m_strIPAddress;
}



void TM_PBX::CommErrorProc(hPbxObject hpbx_obj, void* lpData)
{
	if(lpData == NULL)
		return;
	

	Application::instance().logger().information("PBX:Lost connection with 3300ICP(" +((TM_PBX *)lpData)->m_strIPAddress +"). Please close the tool and try it later! \r\n\r\n"); 
	
		((TM_PBX *)lpData)->m_pSession->RestartSession();
	
	//hpbx_obj = NULL;
	//((TM_PBX *)lpData)->m_hPBX = NULL;
		return;
}

void TM_PBX::GetMonitorState(vector<TM_PBX_MonitorState> & MonState)
{
	std::vector<TM_Address*>::iterator	itAddr = m_vpAddress.begin();
	TM_PBX_MonitorState s;

	//drop existing content
	MonState.clear();

	for (; itAddr < m_vpAddress.end(); itAddr++)
	{
		s.strDN = (*itAddr)->GetDN();
		s.state = (*itAddr)->GetState();
		MonState.push_back(s);
	}
}

/*************TM_Session********************/
TM_Session::TM_Session(string ipAddress, vector<string> dn)
{
	m_bInit = FALSE;
	m_bNeverConnected = TRUE;
	m_strIPAddress = ipAddress;
	m_strDN = dn;
	m_pMiPBX = NULL;
}

TM_Session::~TM_Session()
{
	CloseSession();

}

void TM_Session::OpenSession()

{
	

	if(m_pMiPBX == NULL)
		m_pMiPBX = new TM_PBX(m_strIPAddress, this);


	if(m_pMiPBX != NULL)
	{
			Application::instance().logger().information("Session:Created PBX object successfully!" );
	}else
	{
			Application::instance().logger().information("Session:Failed to create PBX object");
			return;
	}

	//Connecting to PBX
	int iRet = ConnectToICP();

	if (iRet != RESULT_SUCCESS)
	{
		m_bInit = FALSE;
		return;

	}
	
	m_bInit = TRUE;
	m_bNeverConnected = FALSE;

	//Set Monitors

	m_pMiPBX->MonitorLines(m_strDN);
	

}

void TM_Session::CloseSession()
{

	if(m_pMiPBX != NULL)
		delete m_pMiPBX;
	m_pMiPBX = NULL;


	if(m_bInit == TRUE)
	{
		m_bInit = FALSE;
		
	}

	
}


DWORD TM_Session::ConnectToICP()
{
	DWORD dwResult = RESULT_FAIL;

	dwResult = m_pMiPBX->OpenPBX();
	return dwResult;
}

DWORD TM_Session::DisconnectFromICP()
{
	if(m_pMiPBX == NULL)
		return RESULT_FAIL;
	return m_pMiPBX->ClosePBX();
}

void TM_Session::GetSessionStatus(vector<TM_PBX_MonitorState> &MonState)
{
	
	//vector<TM_PBX_MonitorState> MonState;
	if(m_pMiPBX != NULL)
		m_pMiPBX->GetMonitorState(MonState);

}

void TM_Session::RestartSession()
{
	do
	{
		CloseSession();
		Sleep(30000);
		Application::instance().logger().information("Restarting session");
		OpenSession();
		
	}while (m_bInit == FALSE);

	
}

string TM_Session::GetIPAddress()
{
	return m_strIPAddress;
}


MiTAINotification::MiTAINotification(std::string data): _data(data) 
{
}

std::string MiTAINotification::data() const
{
	return _data;
}

/************MiTAISub*****************************/
MiTAISub::MiTAISub(): m_bInit(FALSE)
{
}
MiTAISub::~MiTAISub()
{

}
void MiTAISub::initialize(Application &app)
{
	int iRet = SXERR_FAIL;
	

	if(m_bInit == FALSE)
	{
		iRet = SXInit(SX_COMPACT_MESSAGING,SX_LOG_DIR,"c:\\",NULL);		
		
	}
	if(iRet == SX_OK)
	{
		
		Application::instance().logger().information(std::string(__FUNCTION__) + "Mitai Client Successfully Initialized!");

		m_bInit = TRUE;
		
		Application::instance().logger().information(SXVersion());
		
	}
	else
	{	
		m_bInit = FALSE;
		app.logger().fatal(std::string(__FUNCTION__) + "Failed to initialize Mitai Client. Error Code:" + std::string(SX_ErrorText(iRet)));
		throw Poco::LogicException("Failed to initialize Mitai Client",iRet);
	}
}

void MiTAISub::uninitialize()
{
	int iRet = SXERR_FAIL;
	
	if(m_bInit == TRUE)
	{
		//Close sessions

		std::vector<TM_Session*>::iterator	itSession = m_vpMiSession.begin();

		for (; itSession < m_vpMiSession.end(); itSession++)
		{
			(*itSession)->CloseSession();
			delete (*itSession);
			
		}



		iRet = SXShutdown(NULL);
	
		if(iRet == SX_OK)
			Application::instance().logger().information(std::string(__FUNCTION__) + "Closed the Mitai Session successfully!" );
		else
			Application::instance().logger().information( std::string(__FUNCTION__) + ":Failed to close Mitai Session. Error Code: " + std::string(SX_ErrorText(iRet)));
		
		m_bInit = FALSE;

	}


	if(m_bInit == TRUE)
		iRet = SXShutdown(NULL);
	if(iRet == SX_OK)
	{
		
		Application::instance().logger().information("Mitai Client Successfully Uninitialized!");

		m_bInit = FALSE;
	}
	else
	{
		m_bInit = FALSE;
		Application::instance().logger().fatal("Fatal failure when shutting down MiTAI client. Should newer see this...");
	}

}
const char* MiTAISub::name() const
{
	return "MiTAI";
}
bool MiTAISub::isInitialized()
{
	return m_bInit;
}
void MiTAISub::run()
{
	//Loading ICP configuration into vector
	
	Poco::Util::LayeredConfiguration &_config = Application::instance().config();
	Poco::Util::AbstractConfiguration::Keys keys;
	_config.keys("",keys);

	for (Poco::Util::AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				TM_PBX_Conf record;
				std::string fullKey = "";
				if (!fullKey.empty()) fullKey += '.';
				fullKey.append(*it);
				if (fullKey.find("PBX") != std::string::npos)
				{
					//Application::instance().logger().information(fullKey);
					std::string key = fullKey + "[@ip]";
					record.IPAddress = _config.getString(key);
					Application::instance().logger().information(_config.getString(key));

					Poco::Util::AbstractConfiguration::Keys pbx_keys;
					_config.keys(fullKey,pbx_keys);

					
					for (Poco::Util::AbstractConfiguration::Keys::const_iterator it_pbx = pbx_keys.begin(); it_pbx != pbx_keys.end(); ++it_pbx)
					{
						Application::instance().logger().information(_config.getString(fullKey + "." + *it_pbx + "[@num]"));
						record.TrunkNum.push_back(_config.getString(fullKey + "." + *it_pbx + "[@num]"));
					}
					TM_Config.push_back(record);

				}
			}
	
	std::vector<TM_PBX_Conf>::iterator	itConf = TM_Config.begin();
	for (; itConf < TM_Config.end(); itConf++)
	{
		std::string m_strIPAddress = itConf->IPAddress;
		
		m_vpMiSession.push_back(new TM_Session(m_strIPAddress, itConf->TrunkNum));

		if(m_vpMiSession.back() != NULL)
		{
			m_vpMiSession.back()->OpenSession();
			
		}
	}

}