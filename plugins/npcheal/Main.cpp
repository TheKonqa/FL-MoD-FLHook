#include <windows.h>
#include <stdio.h>
#include <string>
#include <time.h>
#include <FLHook.h>
#include <plugin.h>
#include "header.h"
#include <math.h>

list<INISECTIONVALUE> lstNPC;
list<INISECTIONVALUE> lstNPCIDS;
string set_scNPCFile;
int TimerR;
float MinHealth;
PLUGIN_RETURNCODE returncode;

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scNPCFile = string(szCurDir) + "\\flhook_plugins\\NPCHeal.ini";
	IniGetSection(set_scNPCFile, "NPC", lstNPC);
	TimerR = IniGetI(set_scNPCFile, "Timer","Rate", 1000);
	MinHealth = IniGetF(set_scNPCFile, "Timer","MinHealth", 0.5f);
	lstNPCIDS.clear();
	INISECTIONVALUE lst;
	foreach(lstNPC, INISECTIONVALUE, npclist)
	{
		lst.scKey = itos(CreateID(npclist->scKey.c_str()));
		lst.scValue = npclist->scValue;
		lstNPCIDS.push_back(lst);
	}
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
		{HkTimerNPCRegen,			TimerR,				0},
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
}

void HkTimerNPCRegen()
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
			uint iTarget = 0; 
            pub::SpaceObj::GetTarget(iShip, iTarget);
			if(HkGetClientIDByShip(iTarget))
				return;
			if(iTarget>0)
			{
				uint iTargetShip;
				pub::SpaceObj::GetSolarArchetypeID(iTarget,iTargetShip);
				foreach(lstNPCIDS, INISECTIONVALUE, targets)
			    {
					if(atol(targets->scKey.c_str()) == iTargetShip)
					{
                        float HullNow, MaxHull;
                        float Regen;
                        pub::SpaceObj::GetHealth(iTarget , HullNow, MaxHull);
                        Regen = HullNow / MaxHull;
                        if (Regen<1.0f)
					    {
							if(Regen>MinHealth)
							{
						        pub::SpaceObj::SetRelativeHealth(iTarget, Regen + atof(targets->scValue.c_str()));
							    if(HullNow + atof(targets->scValue.c_str())>=MaxHull-1.0f)
								    pub::SpaceObj::SetRelativeHealth(iTarget,1.0f);
							}
					    }
					}
				}
			}
	     }
}catch(...) { AddLog("Exception in %s", __FUNCTION__);}}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "NPCHeal plugin by kosacid";
	p_PI->sShortName = "NPCHeal";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Update, PLUGIN_HkIServerImpl_Update,0));
	return p_PI;
}