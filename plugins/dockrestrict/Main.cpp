#include <windows.h>
#include <stdio.h>
#include <string>
#include <FLHook.h>
#include <plugin.h>
#include "header.h"


BinaryTree<DOCK_RESTRICTION> *set_btJRestrict;
DOCK_DATA Docking[250];

PLUGIN_RETURNCODE returncode;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;
	list<INISECTIONVALUE> lstValues;
	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	string scPluginCfgFile = string(szCurDir) + "\\flhook_plugins\\DockRestrict.ini";
	IniGetSection(scPluginCfgFile, "DockRestrictions", lstValues);
	set_btJRestrict = new BinaryTree<DOCK_RESTRICTION>();
	foreach(lstValues, INISECTIONVALUE, it4)
	{
		set_btJRestrict->Add(new DOCK_RESTRICTION(CreateID(it4->scKey.c_str()), CreateID((Trim(GetParam(it4->scValue, ',', 0))).c_str()), atoi(Trim(GetParam(it4->scValue, ',', 1)).c_str()), stows(Trim(GetParamToEnd(it4->scValue, ',', 2)))));
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();
	else if(fdwReason == DLL_PROCESS_DETACH)
		delete set_btJRestrict;

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace HkIServerImpl
{    
	EXPORT void __stdcall RequestEvent(int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned long p5, unsigned int p6)
	{
		returncode = DEFAULT_RETURNCODE;
		if(!p1) //docking
		{
			Docking[p6].bCheckedDock = true;
		    if(!HkDockingRestrictions(p6, p3))
				returncode = NOFUNCTIONCALL;
		}
	}

	EXPORT void __stdcall RequestCancel_AFTER(int iType, unsigned int iShip, unsigned int p3, unsigned long p4, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		if(!iType) //docking
		{
			foreach(Docking[iClientID].lstRemCargo, CARGO_REMOVE, cargo)
			{
				HkAddCargo(ARG_CLIENTID(iClientID), cargo->iGoodID, cargo->iCount, false);
			}
			Docking[iClientID].lstRemCargo.clear();
		}

	}
}

EXPORT int __cdecl HkCb_Dock_Call(unsigned int const &uShipID,unsigned int const &uSpaceID,int p3,enum DOCK_HOST_RESPONSE p4)
{
	returncode = NOFUNCTIONCALL;
	if(!p3)
	{
	    uint iClientID = HkGetClientIDByShip(uShipID);
	    if(iClientID)
	    {
		    if(!Docking[iClientID].bCheckedDock)
		    {
			    if(!HkDockingRestrictions(iClientID, uSpaceID))
				    return 0;
		    }
		    else
		        Docking[iClientID].bCheckedDock = false;
	    }
	}
	return pub::SpaceObj::Dock(uShipID, uSpaceID, p3, p4);
}

bool HkDockingRestrictions(uint iClientID, uint iDockObj)
{
	Docking[iClientID].lstRemCargo.clear();
	DOCK_RESTRICTION jrFind = DOCK_RESTRICTION(iDockObj);
	DOCK_RESTRICTION *jrFound = set_btJRestrict->Find(&jrFind);
	if(jrFound)
	{
		list<CARGO_INFO> lstCargo;
		bool bPresent = false;
		int Rem;
		HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, Rem);
		foreach(lstCargo, CARGO_INFO, cargo)
		{
			if(cargo->iArchID == jrFound->iArchID) //Item is present
			{
				if(jrFound->iCount > 0)
				{
					if(cargo->iCount >= jrFound->iCount)
						bPresent = true;
				}
				else if(jrFound->iCount < 0)
				{
					if(cargo->iCount >= -jrFound->iCount)
					{
						bPresent = true;
						CARGO_REMOVE cm;
						cm.iGoodID = cargo->iArchID;
						cm.iCount = -jrFound->iCount;
						Docking[iClientID].lstRemCargo.push_back(cm);
						pub::Player::RemoveCargo(iClientID, cargo->iID, -jrFound->iCount);
					}
				}
				else
				{
					if(cargo->bMounted)
						bPresent = true;
				}
				break;
			}
		}
		if(!bPresent)
		{
			pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("info_access_denied"));
			PrintUserCmdText(iClientID, jrFound->wscDeniedMsg);
			return false; //block dock
		}
	}
	return true;
}

string GetParamToEnd(string scLine, char cSplitChar, uint iPos)
{
	for(uint i = 0, iCurArg = 0; (i < scLine.length()); i++)
	{
		if(scLine[i] == cSplitChar)
		{
			iCurArg++;

			if(iCurArg == iPos)
				return scLine.substr(i + 1);

			while(((i + 1) < scLine.length()) && (scLine[i+1] == cSplitChar))
				i++; // skip "whitechar"
		}
	}

	return "";
}

string Trim(string scIn)
{
	while(scIn.length() && (scIn[0]==' ' || scIn[0]=='	' || scIn[0]=='\n' || scIn[0]=='\r') )
	{
		scIn = scIn.substr(1);
	}
	while(scIn.length() && (scIn[scIn.length()-1]==L' ' || scIn[scIn.length()-1]=='	' || scIn[scIn.length()-1]=='\n' || scIn[scIn.length()-1]=='\r') )
	{
		scIn = scIn.substr(0, scIn.length()-1);
	}
	return scIn;
}

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
 p_PI->sName = "Docking Restrictions plugin by M0tah";
 p_PI->sShortName = "Dock_Restrict";
 p_PI->bMayPause = false;
 p_PI->bMayUnload = false;
 p_PI->ePluginReturnCode = &returncode;
 p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestEvent, PLUGIN_HkIServerImpl_RequestEvent,0));
 p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestCancel_AFTER, PLUGIN_HkIServerImpl_RequestCancel_AFTER,0));
 p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Dock_Call, PLUGIN_HkCb_Dock_Call,0));
 return p_PI;
}