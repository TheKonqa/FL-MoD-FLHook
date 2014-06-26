#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"

list<INISECTIONVALUE> lstReps;
list<INISECTIONVALUE> lstDock;
DOCK_DATA dock[250];

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
    set_scTags = string(szCurDir) + "\\flhook_plugins\\repbytag.ini";
	IniGetSection(set_scTags, "Tags", lstReps);
	IniGetSection(set_scTags, "PlayerDock", lstDock);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

	if(fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();

	return true;
}

HK_ERROR HkRepSet(uint iClientID)
{
    string wscstr = ToLower(wstos(Players.GetActiveCharacterName(iClientID)));
	bool msg=true;
	bool setrep=false;
    foreach(lstReps,INISECTIONVALUE,repset)
    {
       string RepKey1 = "[";
	   RepKey1+=repset->scKey;
       if (wscstr.find(RepKey1) != -1)
	   {
		   setrep=true;
	   }
	   string RepKey2 = "<";
	   RepKey2+=repset->scKey;
       if (wscstr.find(RepKey2) != -1)
	   {
		   setrep=true;
	   }
	   string RepKey3 = "|";
       RepKey3+=repset->scKey;
       if (wscstr.find(RepKey3) != -1)
	   {
		   setrep=true;
	   }
	   string RepKey4 = "(";
       RepKey4+=repset->scKey;
       if (wscstr.find(RepKey4) != -1)
	   {
		   setrep=true;
	   }
       if(setrep)
	   {
		   list<INISECTIONVALUE> lstSetReps;
	       IniGetSection(set_scTags, repset->scKey, lstSetReps);
           foreach(lstSetReps,INISECTIONVALUE,repsetlist)
		   {				
	           uint iRepGroupID = ToInt(stows(repsetlist->scKey));
	           float repf = IniGetF(set_scTags, repset->scKey, repsetlist->scKey, ToFloat(stows(repsetlist->scValue)));
	           pub::Reputation::GetReputationGroup(iRepGroupID, (repsetlist->scKey).c_str());
	           int iPlayerRep;
	           pub::Player::GetRep(iClientID, iPlayerRep);
	           pub::Reputation::SetReputation(iPlayerRep, iRepGroupID, repf);
		   }
	       foreach(lstDock,INISECTIONVALUE,nodock)
		   {
			   uint iSystemID;
	           pub::Player::GetSystem(iClientID, iSystemID);
			   uint iShip; 
               pub::Player::GetShip(iClientID, iShip);
			   uint iTarget; 
               pub::SpaceObj::GetTarget(iShip, iTarget);
		       if(iSystemID == CreateID((nodock->scKey).c_str()))
			   {
			       if(nodock->scValue == repset->scKey)
				   {
                      dock[iClientID].AntiDock=true;
				   }
			   }
			   if(iTarget == CreateID((nodock->scKey).c_str()))
			   {
			       if(nodock->scValue == repset->scKey)
				   {
                      dock[iClientID].AntiDock=true;
				   }
			   }
		   }
	       return HKE_OK;
	   }
	}
return HKE_OK;	
}

namespace HkIServerImpl
{
	EXPORT void __stdcall CharacterSelect_AFTER(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		dock[iClientID].AntiDock=false;
		HkRepSet(iClientID);
	}

	EXPORT void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		HkRepSet(iClientID);
	}

	EXPORT void __stdcall RequestEvent(int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned long p5, unsigned int p6)
	{
		returncode = DEFAULT_RETURNCODE;

		if(!p1)
		{
		    HkRepSet(p6);
		    if(dock[p6].AntiDock)
		    {
			    pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
			    dock[p6].AntiDock=false;
			    returncode = NOFUNCTIONCALL;
		    }
		}
	}
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "set rep by tag plugin by kosacid";
	p_PI->sShortName = "repbytag";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch, PLUGIN_HkIServerImpl_PlayerLaunch,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestEvent, PLUGIN_HkIServerImpl_RequestEvent,0));
	return p_PI;
}