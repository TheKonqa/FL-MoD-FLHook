#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"

BinaryTree<UINT_WRAP> *set_btNoTrade = new BinaryTree<UINT_WRAP>();
BinaryTree<UINT_WRAP> *set_setNoSpaceItems = new BinaryTree<UINT_WRAP>();

PLUGIN_RETURNCODE returncode;

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	list<INISECTIONVALUE> lstValues;
	string set_scCfgItemsFile;
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scCfgItemsFile = string(szCurDir) + "\\flhook_plugins\\Itemrestrictions.ini";

	IniGetSection(set_scCfgItemsFile, "ExcludeTradeItems", lstValues);
	set_btNoTrade->Clear();
	foreach(lstValues, INISECTIONVALUE, it11)
	{
		UINT_WRAP *uw = new UINT_WRAP(CreateID(it11->scKey.c_str()));
		set_btNoTrade->Add(uw);
	}

	IniGetSection(set_scCfgItemsFile, "NoSpaceItems", lstValues);
	set_setNoSpaceItems->Clear();
	foreach(lstValues, INISECTIONVALUE, it16)
	{
		UINT_WRAP *uw = new UINT_WRAP(CreateID(it16->scKey.c_str()));
		set_setNoSpaceItems->Add(uw);
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
	EXPORT void __stdcall AddTradeEquip(unsigned int iClientID, struct EquipDesc const &ed)
	{
		returncode = DEFAULT_RETURNCODE;
		UINT_WRAP uw = UINT_WRAP(ed.iArchID);
		if(set_btNoTrade->Find(&uw))
		{
			pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("objective_failed"));
			Server.TerminateTrade(iClientID, 0); //cancel trade
		}
	}

	EXPORT void __stdcall JettisonCargo(unsigned int iClientID, struct XJettisonCargo const &jc)
	{
		returncode = DEFAULT_RETURNCODE;
		list<CARGO_INFO> lstCargoNew;
		int iRem;
	    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargoNew, iRem);
		foreach(lstCargoNew, CARGO_INFO, cargo)
		{
			if(cargo->iID == jc.iSlot)
			{
			    const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		        if(!gi)
		        continue;
			    UINT_WRAP uw = UINT_WRAP(cargo->iArchID);
			    if(set_setNoSpaceItems->Find(&uw))
			    {
					returncode = NOFUNCTIONCALL;
		            pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("objective_failed"));
				}
			}
		}
	}
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "ItemRestriction plugin by M0tah";
	p_PI->sShortName = "ItemRestriction";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::AddTradeEquip, PLUGIN_HkIServerImpl_AddTradeEquip,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JettisonCargo, PLUGIN_HkIServerImpl_JettisonCargo,0));
	return p_PI;
}

