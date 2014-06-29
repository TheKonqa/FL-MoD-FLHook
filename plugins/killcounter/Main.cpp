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

	string set_scKills;
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scKills = string(szCurDir) + "\\flhook_plugins\\killcounter.ini";
	IniGetSection(set_scKills, "Ranks", lstRanks);
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
	PrintUserCmdText(iClientID, L"/kills <player name>");
	PrintUserCmdText(iClientID, L"/kills$ <player id>");
}

void UserCmd_Kills(uint iClientID, const wstring &wscParam)
{
	wstring wscClientID = GetParam(wscParam, ' ', 0);
	int iNumKills;
	wstring mainrank;
	list<wstring> lstLines;
	int count;
	if(!wscClientID.length())
	{
		wstring wscCharname = Players.GetActiveCharacterName(iClientID);
		HkReadCharFile(wscCharname, lstLines);
		pub::Player::GetNumKills(iClientID, iNumKills);
		PrintUserCmdText(iClientID, L"number of pvp kills = %i",iNumKills);
		foreach(lstLines, wstring, str)
		{
		    if(!ToLower((*str)).find(L"ship_type_killed"))
			{
				uint iShipArchID = ToInt(GetParam(*str, '=', 1).c_str());
				count = ToInt(GetParam(*str, ',', 1).c_str());
				iNumKills+=count;
				Archetype::Ship *ship = Archetype::GetShip(iShipArchID);
				if(!ship)
				continue;
				PrintUserCmdText(iClientID, L"npc kills  %s %i",HkGetWStringFromIDS(ship->iIdsName).c_str(),count);
			}
		}
		foreach(lstRanks, INISECTIONVALUE, rank)
		{
			int lvl = ToInt(rank->scValue);
			if(lvl<=iNumKills)
			{
				mainrank=stows(rank->scKey);
			}
		}
		PrintUserCmdText(iClientID, L"your total kills is = %i" ,iNumKills);
		PrintUserCmdText(iClientID, L"your rank is = %s" ,mainrank.c_str());
		return;
	}

	uint iClientIDPlayer = HkGetClientIdFromCharname(wscClientID);
	if(iClientIDPlayer == -1)
	{
		PrintUserCmdText(iClientID, L"ERROR player not found");
		return;
	}
	HkReadCharFile(wscClientID, lstLines);
	pub::Player::GetNumKills(iClientIDPlayer, iNumKills);
	PrintUserCmdText(iClientID, L"number of pvp kills = %i",iNumKills);
	foreach(lstLines, wstring, str)
	{
		if(!ToLower((*str)).find(L"ship_type_killed"))
		{
			uint iShipArchID = ToInt(GetParam(*str, '=', 1));
			count = ToInt(GetParam(*str, ',', 1).c_str());
			iNumKills+=count;
			Archetype::Ship *ship = Archetype::GetShip(iShipArchID);
			if(!ship)
				continue;
			PrintUserCmdText(iClientID, L"npc kills  %s %i",HkGetWStringFromIDS(ship->iIdsName).c_str(),count);
		}
	}
	foreach(lstRanks, INISECTIONVALUE, rank)
	{
		int lvl = ToInt(rank->scValue);
		if(lvl<=iNumKills)
		{
			mainrank=stows(rank->scKey);
		}
	}
	PrintUserCmdText(iClientID, L"there total kills is = %i" ,iNumKills);
	PrintUserCmdText(iClientID, L"there rank is = %s" ,mainrank.c_str());
}

void UserCmd_Kills$(uint iClientID, const wstring &wscParam)
{
	wstring wscClientTarget = GetParam(wscParam, ' ', 0);
	int iNumKills;
	wstring mainrank;
	list<wstring> lstLines;
	int count;
    uint iClientIDTarget = ToInt(wscClientTarget);
	if(!HkIsValidClientID(iClientIDTarget) || HkIsInCharSelectMenu(iClientIDTarget))
	{
		PrintUserCmdText(iClientID, L"ERROR player not found");
		return;
	}
	wstring wscClientID = (Players.GetActiveCharacterName(iClientIDTarget));
	HkReadCharFile(wscClientID, lstLines);
	pub::Player::GetNumKills(iClientIDTarget, iNumKills);
	PrintUserCmdText(iClientID, L"number of pvp kills = %i",iNumKills);
	foreach(lstLines, wstring, str)
	{
		if(!ToLower((*str)).find(L"ship_type_killed"))
		{
			uint iShipArchID = ToInt(GetParam(*str, '=', 1));
			count = ToInt(GetParam(*str, ',', 1).c_str());
			iNumKills+=count;
			Archetype::Ship *ship = Archetype::GetShip(iShipArchID);
			if(!ship)
				continue;
			PrintUserCmdText(iClientID, L"npc kills  %s %i",HkGetWStringFromIDS(ship->iIdsName).c_str(),count);
		}
	}
	foreach(lstRanks, INISECTIONVALUE, rank)
	{
		int lvl = ToInt(rank->scValue);
		if(lvl<=iNumKills)
		{
			mainrank=stows(rank->scKey);
		}
	}
	PrintUserCmdText(iClientID, L"there total kills is = %i" ,iNumKills);
	PrintUserCmdText(iClientID, L"there rank is = %s" ,mainrank.c_str());
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

EXPORT void __stdcall ShipDestroyed(DamageList *_dmg, char *szECX, uint iKill)
{
	char *szP;
	memcpy(&szP, szECX + 0x10, 4);
	uint iClientID;
	memcpy(&iClientID, szP + 0xB4, 4);
	if(iClientID)
	{
	    DamageList dmg;
	    if(!dmg.get_cause())
		   dmg = ClientInfo[iClientID].dmgLast;
	    uint iClientIDKiller = HkGetClientIDByShip(dmg.get_inflictor_id());
	    if(iClientIDKiller && (iClientID != iClientIDKiller))
	    {
		    int iNumKills;
		    pub::Player::GetNumKills(iClientIDKiller, iNumKills);
		    iNumKills++;
		    pub::Player::SetNumKills(iClientIDKiller, iNumKills);
	    }
    }
}

int ToInt(string scStr)
{
	return atoi(scStr.c_str());
}

typedef void (*_UserCmdProc)(uint, const wstring &);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
};

USERCMD UserCmds[] =
{
	{ L"/kills",					UserCmd_Kills},
	{ L"/kills$",			    UserCmd_Kills$},
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
	p_PI->sName = "kills rank plugin by kosacid";
	p_PI->sShortName = "killcounter";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process,0));
//	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ShipDestroyed, PLUGIN_ShipDestroyed,0));
	return p_PI;
}