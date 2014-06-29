#include <windows.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <FLHook.h>
#include <plugin.h>
#include "header.h"

RGA_DATA regen[250];
list<INISECTIONVALUE> lstArmour;
string set_scRGAFile;

PLUGIN_RETURNCODE returncode;

int stoi(string a)
{
	return atoi(a.c_str());
}

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scRGAFile = string(szCurDir) + "\\flhook_plugins\\regarmour.ini";
	IniGetSection(set_scRGAFile, "Armour", lstArmour);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	LoadSettings();
	return true;
}

namespace HkIServerImpl
{
	typedef void (*_TimerFunc)();
	struct TIMER
	{
		_TimerFunc	proc;
		mstime		tmIntervallMS;
		mstime		tmLastCall;
	};

	TIMER Timers[] = 
	{
		{HkTimerArmourRegen,			1000,				0},
	};

	EXPORT int __stdcall Update()
	{
		returncode = DEFAULT_RETURNCODE;
		for(uint i = 0; (i < sizeof(Timers)/sizeof(TIMER)); i++)
		{
			if((timeInMS() - Timers[i].tmLastCall) >= Timers[i].tmIntervallMS)
			{
				Timers[i].tmLastCall = timeInMS();
				Timers[i].proc();
			}
		}
		return 0;
	}

	EXPORT void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		regen[iClientID].HasArmour = false;
		list <CARGO_INFO> lstCargo;
		int rem;
	    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, rem);
	    foreach(lstCargo, CARGO_INFO, cargo)
		{
	        foreach(lstArmour,INISECTIONVALUE,lstArm)
			{
			    uint iArmour = CreateID(lstArm->scKey.c_str());
				if(iArmour==cargo->iArchID && cargo->bMounted)
				{
					string ShipClass = GetParam(lstArm->scValue,' ',0);
					string RegenTime = GetParam(lstArm->scValue,' ',1);
					int timers=0;
					Archetype::Ship *ship = Archetype::GetShip(Players[iClientID].iShipArchetype);
				    if(ShipClass == itos(ship->iShipClass))
				    {
					    timers = stoi(RegenTime);
					    regen[iClientID].mTime = timers;
					    regen[iClientID].HasArmour = true;
					    regen[iClientID].tmRegenTime = timeInMS() + regen[iClientID].mTime;
					}
				}
			}
		}
	}
	EXPORT void __stdcall CharacterSelect_AFTER(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		regen[iClientID].HasArmour = false;
		list <CARGO_INFO> lstCargo;
		int rem;
	    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, rem);
	    foreach(lstCargo, CARGO_INFO, cargo)
		{
	        foreach(lstArmour,INISECTIONVALUE,lstArm)
			{
			    uint iArmour = CreateID(lstArm->scKey.c_str());
				if(iArmour==cargo->iArchID && cargo->bMounted)
				{
					string ShipClass = GetParam(lstArm->scValue,' ',0);
					string RegenTime = GetParam(lstArm->scValue,' ',1);
					int timers=0;
					Archetype::Ship *ship = Archetype::GetShip(Players[iClientID].iShipArchetype);
				    if(ShipClass == itos(ship->iShipClass))
				    {
					    timers = stoi(RegenTime);
					    regen[iClientID].mTime = timers;
					    regen[iClientID].HasArmour = true;
					    regen[iClientID].tmRegenTime = timeInMS() + regen[iClientID].mTime;
					}
				}
			}
		}
	}
}

void HkTimerArmourRegen()
{
try {
        struct PlayerData *pPD = 0;
        while(pPD = Players.traverse_active(pPD))
		{
			int timers=0;
            uint iClientID = HkGetClientIdFromPD(pPD);
            uint iShip = 0;
	        pub::Player::GetShip(iClientID, iShip);
			if(!iShip )
			{
				return;
			}
			if(regen[iClientID].HasArmour)
			{
			    
			    float maxHealth, curHealth;
			    pub::SpaceObj::GetHealth(ClientInfo[iClientID].iShip, curHealth, maxHealth);
			    if(timeInMS() >= regen[iClientID].tmRegenTime && curHealth < maxHealth)
			    {
                    float HullNow, MaxHull;
                    float Regen;
                    pub::SpaceObj::GetHealth(iShip , HullNow, MaxHull);
                    Regen = HullNow / MaxHull;
					if(regen[iClientID].Repair)
					{
                        if (Regen<=0.9f)
					    {
						    pub::SpaceObj::SetRelativeHealth(iShip, Regen+0.1f); 
					    }
					    else
					    {
						    pub::SpaceObj::SetRelativeHealth(iShip, 1.0f);
						    regen[iClientID].Repair = false;
							return;
					    }
					}
					regen[iClientID].Repair = true;
					regen[iClientID].tmRegenTime = timeInMS() + regen[iClientID].mTime;
				}
			}
	     }
}catch(...) { AddLog("Exception in %s", __FUNCTION__);}}

string GetParam(string scLine, char cSplitChar, uint iPos)
{
	uint i = 0, j = 0;
	string scResult = "";
	for(i = 0, j = 0; (i <= iPos) && (j < scLine.length()); j++)
	{
		if(scLine[j] == cSplitChar)
		{
			while(((j + 1) < scLine.length()) && (scLine[j+1] == cSplitChar))
				j++; // skip "whitechar"
			i++;
			continue;
		}
		if(i == iPos)
			scResult += scLine[j];
	}
	return scResult;
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "regen armour plugin by kosacid";
	p_PI->sShortName = "regarmour";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Update, PLUGIN_HkIServerImpl_Update,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch, PLUGIN_HkIServerImpl_PlayerLaunch,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER,0));
	return p_PI;
}