/**
 Leipzig Compatiability Me Plugin for FLHook - v0.1 by Cannon
 This plugin is based on the based on tempban FHHook plugin by w0dk4.

 This plugin provides two admin commands for FLHook Plugin that provide
 exactly the same behaviour as FLHook LeipzigCity 1.5.6. 

 Equivalent command  are available in FLHook Plugin 1.5.9 by the command
 string and the output differ slightly.

 These commands are used by CD (cheater's death) anti-cheating SW
*/

// includes 
#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"

PLUGIN_RETURNCODE returncode;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CmdGetClientId(CCmds* cmds, const wstring &wscCharname)
{
	uint iClientID = HkGetClientIdFromCharname(wscCharname);
	if(iClientID == -1)
	{
		cmds->hkLastErr = HKE_PLAYER_NOT_LOGGED_IN;
		cmds->PrintError();
		return;
	}

	cmds->Print(L"clientid=%u\nOK\n", iClientID);
}

void CmdEnumEQList(CCmds* cmds, const wstring &wscCharname)
{
	if(!(cmds->rights & RIGHT_SPECIAL3)) {
		cmds->Print(L"ERR No permission\n");
		return;
	}

	int remainingHoldSize;
	list<CARGO_INFO> lstCargo;
	wstring wstrEqList(L"");
	bool bHasMounted=false;
	if((cmds->hkLastErr=HkEnumCargo(wscCharname, lstCargo,remainingHoldSize))==HKE_OK) {
		foreach(lstCargo, CARGO_INFO, it)
		{
			if((*it).bMounted)
			{
				wchar_t wszTemp[256];
				swprintf(wszTemp, L"id=%2.2x archid=%8.8x\n", (*it).iID, (*it).iArchID);
				wstrEqList.append(wszTemp);
				bHasMounted=true;
			}

		}
		if(bHasMounted)
			cmds->Print(L"%s", wstrEqList.c_str());
		cmds->Print(L"OK\n");
	} else
		cmds->PrintError();
}

#define IS_CMD(a) !wscCmd.compare(L##a)

EXPORT bool ExecuteCommandString_Callback(CCmds* classptr, const wstring &wscCmd)
{
	returncode = NOFUNCTIONCALL;

	if(IS_CMD("enumeqlist")) {
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		CmdEnumEQList(classptr, classptr->ArgCharname(1));
		return true;
	}
	if(IS_CMD("getclientid")) {
		returncode = SKIPPLUGINS_NOFUNCTIONCALL; 
		CmdGetClientId(classptr, classptr->ArgCharname(1));
		return true;
	}

    return false;
}

EXPORT void CmdHelp_Callback(CCmds* classptr)
{
	returncode = DEFAULT_RETURNCODE;
	classptr->Print(L"enumeqlist <charname>\n");
	classptr->Print(L"getclientid <charname>\n");
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "Lzcompat Plugin v0.1 by Cannon";
	p_PI->sShortName = "lzcompat";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CmdHelp_Callback, PLUGIN_CmdHelp_Callback,0));
	return p_PI;
}