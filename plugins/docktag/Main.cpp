#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"

list<INISECTIONVALUE> lstTags;
list<INISECTIONVALUE> lstDock;
DOCK_DATA docking[250];

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
    set_scDock = string(szCurDir) + "\\flhook_plugins\\docktag.ini";
	IniGetSection(set_scDock, "Tags", lstTags);
	IniGetSection(set_scDock, "Docking", lstDock);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

	if(fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();

	return true;
}

void Dock(uint iClientID, uint DockID)
{
	string wscstr = ToLower(wstos(Players.GetActiveCharacterName(iClientID)));
	foreach(lstTags, INISECTIONVALUE, tag)
	{
		bool Found = false;
		string Key1 = "[";
	    Key1+=tag->scKey;
        if (wscstr.find(Key1) != -1)
	    {
			Found=true;
	    }
		string Key2 = "<";
	    Key2+=tag->scKey;
        if (wscstr.find(Key2) != -1)
	    {
		    Found=true;
	    }
	    string Key3 = "|";
        Key3+=tag->scKey;
        if (wscstr.find(Key3) != -1)
	    {
		    Found=true;
	    }
	    string Key4 = "(";
        Key4+=tag->scKey;
        if (wscstr.find(Key4) != -1)
	    {
		    Found=true;
	    }
		if(Found)
		{
		    list<INISECTIONVALUE> lstDock;
		    IniGetSection(set_scDock, tag->scKey, lstDock);
		    foreach(lstDock, INISECTIONVALUE, lst)
		    {
			    uint ID = CreateID(lst->scKey.c_str());
			    if(DockID == ID)
			    {
				    docking[iClientID].Dock=true;
			    }
		    }
		}
	}
}

namespace HkIServerImpl
{
	EXPORT void __stdcall CharacterSelect_AFTER(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		docking[iClientID].Dock=false;
	}

	EXPORT void __stdcall RequestEvent(int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned long p5, unsigned int p6)
	{
		returncode = DEFAULT_RETURNCODE;
		bool LetDock = true;
		if(!p1)
		{
		    foreach(lstDock, INISECTIONVALUE, dockwith)
		    {
			    uint DockID = CreateID(dockwith->scKey.c_str());
			    if(DockID == p3)
			    {
				    LetDock = false;
					Dock(p6,p3);
					LetDock = docking[p6].Dock;
			    }
			}
		}

		if(!LetDock)
		{
			pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
			returncode = NOFUNCTIONCALL;
		}
		docking[p6].Dock=false;
	}
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "docktag plugin by kosacid";
	p_PI->sShortName = "docktag";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestEvent, PLUGIN_HkIServerImpl_RequestEvent,0));
	return p_PI;
}