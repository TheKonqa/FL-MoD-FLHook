#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"
#include <vector>
#define ADDR_FLCONFIG 0x25410

vector<HINSTANCE> vDLLs;
PLUGIN_RETURNCODE returncode;

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	char *szFLConfig = (char*)((char*)GetModuleHandle(0) + ADDR_FLCONFIG);
	HkLoadDLLConf(szFLConfig);
}

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

	if(fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();

	return true;
}

EXPORT void UserCmd_Help(uint iClientID, const wstring &wscParam)
{
	PrintUserCmdText(iClientID, L"/bequip <--- no prams will supply a list /bequip <n> will build the item on the list");
	PrintUserCmdText(iClientID, L"/bwep <--- no prams will supply a list /bwep <n> will build the item on the list");
	PrintUserCmdText(iClientID, L"/bammo <--- no prams will supply a list /bammo <n> will build the item on the list");
}

void UserCmd_eBuild(uint iClientID, const wstring &wscParam)
{
	string set_scBuildFilee;
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	set_scBuildFilee = string(szCurDir) + "\\flhook_plugins\\builde.ini";

	uint iShip = 0;
	pub::Player::GetShip(iClientID, iShip);
	if(!iShip)
	{
		PrintUserCmdText(iClientID, L"You cant use that here");
		return;
	}
    int counter=0;
	int countgoods = 0;
	int countgoodsT = 0;
	int BuildNumber = ToInt(GetParam(wscParam, ' ', 0));
	list<INISECTIONVALUE> lstBuilds;
	list<INISECTIONVALUE> lstParts;
	list<CARGO_INFO> lstCargo;
	IniGetSection(set_scBuildFilee, "build", lstBuilds);
	if(wscParam.length()>0)
	{
		foreach(lstBuilds,INISECTIONVALUE,builds)
		{
		    if(counter==BuildNumber)
			{
				IniGetSection(set_scBuildFilee, builds->scKey.c_str(), lstParts);
		        foreach(lstParts,INISECTIONVALUE,parts)
				{
					uint PartsID = CreateID(parts->scKey.c_str());
					countgoodsT ++;
					int iRem;
	                HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
	                uint iNum = 0;
	                foreach(lstCargo, CARGO_INFO, cargo)
					{
		                const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		                if(!gi)
		                continue;
		                if(!cargo->bMounted && gi->iIDS)
						{
			                iNum++;
							if(cargo->iArchID==PartsID && cargo->iCount>=ToInt(stows(parts->scValue)))
							{
								countgoods ++;
							}
						}
					}
	                if(!iNum)
					{
	                    PrintUserCmdText(iClientID, L"Error: you have no equipment or goods to build this");
						return;
					}
				}
			}
			counter++;
		}
		if(countgoods==countgoodsT)
		{
			counter=0;
			foreach(lstBuilds,INISECTIONVALUE,builds)
			{
				uint wscGoods = CreateID(builds->scKey.c_str());
				if(counter==BuildNumber)
				{
					int wscCount = 1;
					Archetype::Equipment *eq = Archetype::GetEquipment(wscGoods);
				    const GoodInfo *id = GoodList::find_by_archetype(wscGoods);
					if(!id)
						continue;
				    float fRemainingHold;
				    pub::Player::GetRemainingHoldSize(iClientID, fRemainingHold);
				    if(id->iType == 0)
					{
		                if(eq->fVolume*wscCount > fRemainingHold)
						{
					        PrintUserCmdText(iClientID, L"You dont have enough cargo space");
					        wscCount = 0;
						}
					}
				    int nCount=0;
				    int sCount=0;
				    uint iNanobotsID = 2911012559;
	                uint iShieldBatID = 2596081674;
	                Archetype::Ship *ship = Archetype::GetShip(Players[iClientID].iShipArchetype);
				    list<CARGO_INFO> lstCargo;
					int iRem;
			        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
			        foreach(lstCargo, CARGO_INFO, cargo)
					{
				        if(cargo->iArchID == iNanobotsID){nCount = cargo->iCount;}
				        if(cargo->iArchID == iShieldBatID){sCount = cargo->iCount;}
                        if(wscGoods == iNanobotsID)
						{
						    uint amount = nCount+wscCount;
			                if(amount > ship->iMaxNanobots)
							{
				                PrintUserCmdText(iClientID, L"Warning: the number of nanobots is greater than allowed");
						        wscCount = 0;
							}
						}
                        if(wscGoods == iShieldBatID)
						{
                            uint amount = sCount+wscCount;
			                if(amount > ship->iMaxShieldBats)
							{
				                PrintUserCmdText(iClientID, L"Warning: the number of shield batteries is greater than allowed");
						        wscCount = 0;
							}
						}
					}
				    if(id->iType == 1)
					{
					    int uCount=0;
					    foreach(lstCargo, CARGO_INFO, cargo)
						{
						    if(cargo->iArchID == wscGoods){uCount = cargo->iCount;}
						    if(wscCount+uCount > MAX_PLAYER_AMMO)
							{
							    PrintUserCmdText(iClientID, L"Warning: the number of goods is greater than allowed");
							    wscCount=0;
							}
						    if(eq->fVolume*wscCount > fRemainingHold)
							{
					            PrintUserCmdText(iClientID, L"You dont have enough cargo space");
					            wscCount = 0;
							}
						}
					}
					if(wscCount==1)
					{
						IniGetSection(set_scBuildFilee, builds->scKey.c_str(), lstParts);
					    foreach(lstParts,INISECTIONVALUE,parts)
						{
							foreach(lstCargo, CARGO_INFO, cargo)
							{
							    uint PartsID = CreateID(parts->scKey.c_str());
						        int iCargo = ToInt(stows(parts->scValue));
								if(PartsID==cargo->iArchID)
								{
						            HkRemoveCargo(ARG_CLIENTID(iClientID), cargo->iID, iCargo);
								}
							}
						}
					    HkAddCargo(ARG_CLIENTID(iClientID), wscGoods, wscCount, false);
						PrintUserCmdText(iClientID, L"Item Built");
					}
					Archetype::Gun *gun = (Archetype::Gun *)eq;
					if(gun->iArchID)//if not here items will stack and not show the amount
					{
						PrintUserCmdText(iClientID, L"Ok"); 
					}
				}
				counter++;
			}
			PrintUserCmdText(iClientID, L"OK");
		}
		else
		{
			PrintUserCmdText(iClientID, L"You dont have enough materials to build that");
		}
		return;
	}
	string List="";
	foreach(lstBuilds,INISECTIONVALUE,builds)
	{
		char buffer[256]="";
		uint BuildsID = CreateID(builds->scKey.c_str());
		IniGetSection(set_scBuildFilee, builds->scKey.c_str(), lstParts);
		const GoodInfo *gi = GoodList::find_by_id(BuildsID);
		sprintf(buffer,"(%i)%s-->", counter, wstos(HkGetWStringFromIDS(gi->iIDSName)).c_str());
		List+=buffer;
		counter++;
		foreach(lstParts,INISECTIONVALUE,parts)
		{
			char buffer1[256]="";
			uint PartsID = CreateID(parts->scKey.c_str());
			const GoodInfo *gp = GoodList::find_by_id(PartsID);
			sprintf(buffer1," %s=%i ", wstos(HkGetWStringFromIDS(gp->iIDSName)).c_str(), ToInt(stows(parts->scValue)));
			if(List.length() + 256 >= 1024)
			{
				PrintUserCmdText(iClientID, L"%s",stows(List).c_str());
				List="";
			}
			List+=buffer1;
		}
	}
	PrintUserCmdText(iClientID, L"%s",stows(List).c_str());
}

void UserCmd_wBuild(uint iClientID, const wstring &wscParam)
{
	string set_scBuildFilew;
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	set_scBuildFilew = string(szCurDir) + "\\flhook_plugins\\buildw.ini";

	uint iShip = 0;
	pub::Player::GetShip(iClientID, iShip);
	if(!iShip)
	{
		PrintUserCmdText(iClientID, L"You cant use that here");
		return;
	}
    int counter=0;
	int countgoods = 0;
	int countgoodsT = 0;
	int BuildNumber = ToInt(GetParam(wscParam, ' ', 0));
	list<INISECTIONVALUE> lstBuilds;
	list<INISECTIONVALUE> lstParts;
	list<CARGO_INFO> lstCargo;
	IniGetSection(set_scBuildFilew, "build", lstBuilds);
	if(wscParam.length()>0)
	{
		foreach(lstBuilds,INISECTIONVALUE,builds)
		{
		    if(counter==BuildNumber)
			{
				IniGetSection(set_scBuildFilew, builds->scKey.c_str(), lstParts);
		        foreach(lstParts,INISECTIONVALUE,parts)
				{
					uint PartsID = CreateID(parts->scKey.c_str());
					countgoodsT ++;
					int iRem;
	                HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
	                uint iNum = 0;
	                foreach(lstCargo, CARGO_INFO, cargo)
					{
		                const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		                if(!gi)
		                continue;
		                if(!cargo->bMounted && gi->iIDS)
						{
			                iNum++;
							if(cargo->iArchID==PartsID && cargo->iCount>=ToInt(stows(parts->scValue)))
							{
								countgoods ++;
							}
						}
					}
	                if(!iNum)
					{
	                    PrintUserCmdText(iClientID, L"Error: you have no equipment or goods to build this");
						return;
					}
				}
			}
			counter++;
		}
		if(countgoods==countgoodsT)
		{
			counter=0;
			foreach(lstBuilds,INISECTIONVALUE,builds)
			{
				uint wscGoods = CreateID(builds->scKey.c_str());
				if(counter==BuildNumber)
				{
					int wscCount = 1;
					Archetype::Equipment *eq = Archetype::GetEquipment(wscGoods);
				    const GoodInfo *id = GoodList::find_by_archetype(wscGoods);
					if(!id)
						continue;
				    float fRemainingHold;
				    pub::Player::GetRemainingHoldSize(iClientID, fRemainingHold);
				    if(id->iType == 0)
					{
		                if(eq->fVolume*wscCount > fRemainingHold)
						{
					        PrintUserCmdText(iClientID, L"You dont have enough cargo space");
					        wscCount = 0;
						}
					}
				    int nCount=0;
				    int sCount=0;
				    uint iNanobotsID = 2911012559;
	                uint iShieldBatID = 2596081674;
	                Archetype::Ship *ship = Archetype::GetShip(Players[iClientID].iShipArchetype);
				    list<CARGO_INFO> lstCargo;
					int iRem;
			        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
			        foreach(lstCargo, CARGO_INFO, cargo)
					{
				        if(cargo->iArchID == iNanobotsID){nCount = cargo->iCount;}
				        if(cargo->iArchID == iShieldBatID){sCount = cargo->iCount;}
                        if(wscGoods == iNanobotsID)
						{
						    uint amount = nCount+wscCount;
			                if(amount > 0)
							{
				                PrintUserCmdText(iClientID, L"Warning: the number of nanobots is greater than allowed");
						        wscCount = ship->iMaxNanobots-nCount;
							}
						}
                        if(wscGoods == iShieldBatID)
						{
                            uint amount = sCount+wscCount;
			                if(amount > ship->iMaxShieldBats)
							{
				                PrintUserCmdText(iClientID, L"Warning: the number of shield batteries is greater than allowed");
						        wscCount = 0;
							}
						}
					}
				    if(id->iType == 1)
					{
					    int uCount=0;
					    foreach(lstCargo, CARGO_INFO, cargo)
						{
						    if(cargo->iArchID == wscGoods){uCount = cargo->iCount;}
						    if(wscCount+uCount > MAX_PLAYER_AMMO)
							{
							    PrintUserCmdText(iClientID, L"Warning: the number of goods is greater than allowed");
							    wscCount=0;
							}
						    if(eq->fVolume*wscCount > fRemainingHold)
							{
					            PrintUserCmdText(iClientID, L"You dont have enough cargo space");
					            wscCount = 0;
							}
						}
					}
					if(wscCount==1)
					{
						IniGetSection(set_scBuildFilew, builds->scKey.c_str(), lstParts);
					    foreach(lstParts,INISECTIONVALUE,parts)
						{
							foreach(lstCargo, CARGO_INFO, cargo)
							{
							    uint PartsID = CreateID(parts->scKey.c_str());
						        int iCargo = ToInt(stows(parts->scValue));
								if(PartsID==cargo->iArchID)
								{
						            HkRemoveCargo(ARG_CLIENTID(iClientID), cargo->iID, iCargo);
								}
							}
						}
					    HkAddCargo(ARG_CLIENTID(iClientID), wscGoods, wscCount, false);
						PrintUserCmdText(iClientID, L"Item Built");
					}
					Archetype::Gun *gun = (Archetype::Gun *)eq;
					if(gun->iArchID)//if not here items will stack and not show the amount
					{
						PrintUserCmdText(iClientID, L"Ok");
					}
				}
				counter++;
			}
			PrintUserCmdText(iClientID, L"OK");
		}
		else
		{
			PrintUserCmdText(iClientID, L"You dont have enough materials to build that");
		}
		return;
	}
	string List="";
	foreach(lstBuilds,INISECTIONVALUE,builds)
	{
		char buffer[256]="";
		uint BuildsID = CreateID(builds->scKey.c_str());
		IniGetSection(set_scBuildFilew, builds->scKey.c_str(), lstParts);
		const GoodInfo *gi = GoodList::find_by_id(BuildsID);
		sprintf(buffer,"(%i)%s-->", counter, wstos(HkGetWStringFromIDS(gi->iIDSName)).c_str());
		List+=buffer;
		counter++;
		foreach(lstParts,INISECTIONVALUE,parts)
		{
			char buffer1[256]="";
			uint PartsID = CreateID(parts->scKey.c_str());
			const GoodInfo *gp = GoodList::find_by_id(PartsID);
		    sprintf(buffer1," %s=%i ", wstos(HkGetWStringFromIDS(gp->iIDSName)).c_str(), ToInt(stows(parts->scValue)));
			if(List.length() + 256 >= 1024)
			{
				PrintUserCmdText(iClientID, L"%s",stows(List).c_str());
				List="";
			}
			List+=buffer1;
		}
	}
	PrintUserCmdText(iClientID, L"%s",stows(List).c_str());
}

void UserCmd_aBuild(uint iClientID, const wstring &wscParam)
{
	string set_scBuildFilea;
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	set_scBuildFilea = string(szCurDir) + "\\flhook_plugins\\builda.ini";

	uint iShip = 0;
	pub::Player::GetShip(iClientID, iShip);
	if(!iShip)
	{
		PrintUserCmdText(iClientID, L"You cant use that here");
		return;
	}
    int counter=0;
	int countgoods = 0;
	int countgoodsT = 0;
	int BuildNumber = ToInt(GetParam(wscParam, ' ', 0));
	list<INISECTIONVALUE> lstBuilds;
	list<INISECTIONVALUE> lstParts;
	list<CARGO_INFO> lstCargo;
	IniGetSection(set_scBuildFilea, "build", lstBuilds);
	if(wscParam.length()>0)
	{
		foreach(lstBuilds,INISECTIONVALUE,builds)
		{
		    if(counter==BuildNumber)
			{
				IniGetSection(set_scBuildFilea, builds->scKey.c_str(), lstParts);
		        foreach(lstParts,INISECTIONVALUE,parts)
				{
					uint PartsID = CreateID(parts->scKey.c_str());
					countgoodsT ++;
					int iRem;
	                HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
	                uint iNum = 0;
	                foreach(lstCargo, CARGO_INFO, cargo)
					{
		                const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		                if(!gi)
		                continue;
		                if(!cargo->bMounted && gi->iIDS)
						{
			                iNum++;
							if(cargo->iArchID==PartsID && cargo->iCount>=ToInt(stows(parts->scValue)))
							{
								countgoods ++;
							}
						}
					}
	                if(!iNum)
					{
	                    PrintUserCmdText(iClientID, L"Error: you have no equipment or goods to build this");
						return;
					}
				}
			}
			counter++;
		}
		if(countgoods==countgoodsT)
		{
			counter=0;
			foreach(lstBuilds,INISECTIONVALUE,builds)
			{
				uint wscGoods = CreateID(builds->scKey.c_str());
				if(counter==BuildNumber)
				{
					int wscCount = 1;
					Archetype::Equipment *eq = Archetype::GetEquipment(wscGoods);
				    const GoodInfo *id = GoodList::find_by_archetype(wscGoods);
					if(!id)
						continue;
				    float fRemainingHold;
				    pub::Player::GetRemainingHoldSize(iClientID, fRemainingHold);
				    if(id->iType == 0)
					{
		                if(eq->fVolume*wscCount > fRemainingHold)
						{
					        PrintUserCmdText(iClientID, L"You dont have enough cargo space");
					        wscCount = 0;
						}
					}
				    int nCount=0;
				    int sCount=0;
				    uint iNanobotsID = 2911012559;
	                uint iShieldBatID = 2596081674;
	                Archetype::Ship *ship = Archetype::GetShip(Players[iClientID].iShipArchetype);
				    list<CARGO_INFO> lstCargo;
					int iRem;
			        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
			        foreach(lstCargo, CARGO_INFO, cargo)
					{
				        if(cargo->iArchID == iNanobotsID){nCount = cargo->iCount;}
				        if(cargo->iArchID == iShieldBatID){sCount = cargo->iCount;}
                        if(wscGoods == iNanobotsID)
						{
						    uint amount = nCount+wscCount;
			                if(amount > ship->iMaxNanobots)
							{
				                PrintUserCmdText(iClientID, L"Warning: the number of nanobots is greater than allowed");
						        wscCount = 0;
							}
						}
                        if(wscGoods == iShieldBatID)
						{
                            uint amount = sCount+wscCount;
			                if(amount > ship->iMaxShieldBats)
							{
				                PrintUserCmdText(iClientID, L"Warning: the number of shield batteries is greater than allowed");
						        wscCount = 0;
							}
						}
					}
				    if(id->iType == 1)
					{
					    int uCount=0;
					    foreach(lstCargo, CARGO_INFO, cargo)
						{
						    if(cargo->iArchID == wscGoods){uCount = cargo->iCount;}
						    if(wscCount+uCount > MAX_PLAYER_AMMO)
							{
							    PrintUserCmdText(iClientID, L"Warning: the number of goods is greater than allowed");
							    wscCount=0;
							}
						    if(eq->fVolume*wscCount > fRemainingHold)
							{
					            PrintUserCmdText(iClientID, L"You dont have enough cargo space");
					            wscCount = 0;
							}
						}
					}
					if(wscCount==1)
					{
						IniGetSection(set_scBuildFilea, builds->scKey.c_str(), lstParts);
					    foreach(lstParts,INISECTIONVALUE,parts)
						{
							foreach(lstCargo, CARGO_INFO, cargo)
							{
							    uint PartsID = CreateID(parts->scKey.c_str());
						        int iCargo = ToInt(stows(parts->scValue));
								if(PartsID==cargo->iArchID)
								{
						            HkRemoveCargo(ARG_CLIENTID(iClientID), cargo->iID, iCargo);
								}
							}
						}
					    HkAddCargo(ARG_CLIENTID(iClientID), wscGoods, wscCount, false);
						PrintUserCmdText(iClientID, L"Item Built");
					}
					Archetype::Gun *gun = (Archetype::Gun *)eq;
					if(gun->iArchID)//if not here items will stack and not show the amount
					{
						PrintUserCmdText(iClientID, L"Ok");
					}
				}
				counter++;
			}
			PrintUserCmdText(iClientID, L"OK");
		}
		else
		{
			PrintUserCmdText(iClientID, L"You dont have enough materials to build that");
		}
		return;
	}
	string List="";
	foreach(lstBuilds,INISECTIONVALUE,builds)
	{
		char buffer[256]="";
		uint BuildsID = CreateID(builds->scKey.c_str());
		IniGetSection(set_scBuildFilea, builds->scKey.c_str(), lstParts);
		const GoodInfo *gi = GoodList::find_by_id(BuildsID);
		sprintf(buffer,"(%i)%s-->", counter, wstos(HkGetWStringFromIDS(gi->iIDSName)).c_str());
		List+=buffer;
		counter++;
		foreach(lstParts,INISECTIONVALUE,parts)
		{
			char buffer1[256]="";
			uint PartsID = CreateID(parts->scKey.c_str());
			const GoodInfo *gp = GoodList::find_by_id(PartsID);
			sprintf(buffer1," %s=%i ", wstos(HkGetWStringFromIDS(gp->iIDSName)).c_str(), ToInt(stows(parts->scValue)));
			if(List.length() + 256 >= 1024)
			{
				PrintUserCmdText(iClientID, L"%s",stows(List).c_str());
				List="";
			}
			List+=buffer1;
		}
	}
	PrintUserCmdText(iClientID, L"%s",stows(List).c_str());
}

wstring HkGetWStringFromIDS(uint iIDS) //Only works for names
{
	if(!iIDS)
		return L"";

	uint iDLL = iIDS / 0x10000;
	iIDS -= iDLL * 0x10000;

	wchar_t wszBuf[512];
	if(LoadStringW(vDLLs[iDLL], iIDS, wszBuf, 512))
		return wszBuf;
	return L"";
}

void HkLoadDLLConf(const char *szFLConfigFile)
{
	for(uint i=0; i<vDLLs.size(); i++)
	{
		FreeLibrary(vDLLs[i]);
	}
	vDLLs.clear();
	HINSTANCE hDLL = LoadLibraryEx((char*)((char*)GetModuleHandle(0) + 0x256C4), NULL, LOAD_LIBRARY_AS_DATAFILE); //typically resources.dll
	if(hDLL)
		vDLLs.push_back(hDLL);
	INI_Reader ini;
	if(ini.open(szFLConfigFile, false))
	{
		while(ini.read_header())
		{
			if(ini.is_header("Resources"))
			{
				while(ini.read_value())
				{
					if(ini.is_value("DLL"))
					{
						hDLL = LoadLibraryEx(ini.get_value_string(0), NULL, LOAD_LIBRARY_AS_DATAFILE);
						if(hDLL)
							vDLLs.push_back(hDLL);
					}
				}
			}
		}
		ini.close();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*_UserCmdProc)(uint, const wstring &);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
};

USERCMD UserCmds[] =
{
	{ L"/bequip",				UserCmd_eBuild},
	{ L"/bwep",			        UserCmd_wBuild},
	{ L"/bammo",			    UserCmd_aBuild},
};

EXPORT bool UserCmd_Process(uint iClientID, const wstring &wscCmd)
{

	wstring wscCmdLower = ToLower(wscCmd);

	
	for(uint i = 0; (i < sizeof(UserCmds)/sizeof(USERCMD)); i++)
	{
		if(wscCmdLower.find(ToLower(UserCmds[i].wszCmd)) == 0)
		{
			wstring wscParam = L"";
			if(wscCmd.length() > wcslen(UserCmds[i].wszCmd))
			{
				if(wscCmd[wcslen(UserCmds[i].wszCmd)] != ' ')
					continue;
				wscParam = wscCmd.substr(wcslen(UserCmds[i].wszCmd) + 1);
			}
			UserCmds[i].proc(iClientID, wscParam);

			returncode = SKIPPLUGINS_NOFUNCTIONCALL; // we handled the command, return immediatly
			return true;
		}
	}
	
	returncode = DEFAULT_RETURNCODE; // we did not handle the command, so let other plugins or FLHook kick in
	return false;
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "build goods plugin by kosacid";
	p_PI->sShortName = "build";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	return p_PI;
}