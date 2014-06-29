#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <FLHook.h>
#include <plugin.h>
#include "header.h"
#include <vector>
#define ADDR_FLCONFIG 0x25410

vector<HINSTANCE> vDLLs;

PLUGIN_RETURNCODE returncode;


EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	char *szFLConfig = (char*)((char*)GetModuleHandle(0) + ADDR_FLCONFIG);
	HkLoadDLLConf(szFLConfig);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

	if(fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();

	return true;
}

EXPORT void UserCmd_Help(uint iClientID, const wstring &wscParam)
{
	PrintUserCmdText(iClientID, L"/list <player name>");
}

void UserCmd_List(uint iClientID, const wstring &wscParam)
{
	if(wscParam.length()>0)
	{
		uint uiTargetID = HkGetClientIdFromCharname(wscParam);
		if(uiTargetID == -1 || HkIsInCharSelectMenu(uiTargetID) )
		{
		    PrintUserCmdText(iClientID, L"Error: %s not found", wscParam.c_str());
		    return;
		}
		wstring wscPlayer = (wchar_t*)Players.GetActiveCharacterName(uiTargetID);
		wstring wscShip = HkGetWStringFromIDS(Archetype::GetShip(Players[uiTargetID].iShipArchetype)->iIdsName);
		wstring Faction;
		HkGetAffiliation(wscPlayer,Faction);
		int FactionLess = strlen((char*)Faction.c_str());
		if (FactionLess!=0)
		{
			PrintUserCmdText(iClientID, L"%s [%s] %s",wscPlayer.c_str(),Faction.c_str(),wscShip.c_str());
		}
		else
		{
		    PrintUserCmdText(iClientID, L"%s [%s] %s",wscPlayer.c_str(),L"unknown",wscShip.c_str());
		}
	}
	else
	{
		struct PlayerData *pPD = 0;
		while(pPD = Players.traverse_active(pPD))
		{
			uint iPlayerID = HkGetClientIdFromPD(pPD);
			if(ClientInfo[iPlayerID].tmF1TimeDisconnect)
				continue;
			wstring wscPlayer = (wchar_t*)Players.GetActiveCharacterName(iPlayerID);
			wstring wscShip = HkGetWStringFromIDS(Archetype::GetShip(Players[iPlayerID].iShipArchetype)->iIdsName);
			wstring Faction;
			HkGetAffiliation(wscPlayer,Faction);
			int FactionLess = strlen((char*)Faction.c_str());
		    if (FactionLess!=0)
			{
			    PrintUserCmdText(iClientID, L"%s [%s] %s",wscPlayer.c_str(),Faction.c_str(),wscShip.c_str());
			}
		    else
			{
		        PrintUserCmdText(iClientID, L"%s [%s] %s",wscPlayer.c_str(),L"unknown",wscShip.c_str());
			}
		}
	}
}

HK_ERROR HkGetAffiliation(wstring wscCharname, wstring &wscRepGroup)
{
	list<wstring> lstLines;
	HK_ERROR hErr = HkReadCharFile(wscCharname, lstLines);
	if(!HKHKSUCCESS(hErr))
		return hErr;
	
	foreach(lstLines, wstring, str)
	{
		if(!ToLower((*str)).find(L"rep_group")) //Line contains affiliation
		{
			wscRepGroup = (*str).substr((*str).find(L"=")+1);
			wscRepGroup = Trim(wscRepGroup);
		}
	}
	uint iRepGroupID;
	pub::Reputation::GetReputationGroup(iRepGroupID, wstos(wscRepGroup).c_str());
    uint iNameID;
	pub::Reputation::GetGroupName(iRepGroupID, iNameID);
	wscRepGroup = HkGetWStringFromIDS(iNameID);
	return HKE_OK;
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

wstring Trim(wstring wscIn)
{
	while(wscIn.length() && (wscIn[0]==L' ' || wscIn[0]==L'	' || wscIn[0]==L'\n' || wscIn[0]==L'\r') )
	{
		wscIn = wscIn.substr(1);
	}
	while(wscIn.length() && (wscIn[wscIn.length()-1]==L' ' || wscIn[wscIn.length()-1]==L'	' || wscIn[wscIn.length()-1]==L'\n' || wscIn[wscIn.length()-1]==L'\r') )
	{
		wscIn = wscIn.substr(0, wscIn.length()-1);
	}
	return wscIn;
}

typedef void (*_UserCmdProc)(uint, const wstring &);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
};

USERCMD UserCmds[] =
{
	{ L"/list",					UserCmd_List},
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
	p_PI->sName = "list players info plugin by kosacid";
	p_PI->sShortName = "list";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process,0));
//	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	return p_PI;
}