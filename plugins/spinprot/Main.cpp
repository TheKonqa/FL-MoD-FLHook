#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"

float set_fSpinProtectMass;
float set_fSpinImpulseMultiplier;

PLUGIN_RETURNCODE returncode;

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	string set_scCfgSpinFile;
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scCfgSpinFile = string(szCurDir) + "\\flhook_plugins\\SpinProtection.ini";
	set_fSpinProtectMass = IniGetF(set_scCfgSpinFile, "General", "SpinProtectionMass", -1.0f);
	set_fSpinImpulseMultiplier = IniGetF(set_scCfgSpinFile, "General", "SpinProtectionMultiplier", -8.0f);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	LoadSettings();
	return true;
}

namespace HkIServerImpl
{ 
    EXPORT void __stdcall SPObjCollision(struct SSPObjCollisionInfo const &ci, unsigned int iClientID)
    {
	    returncode = DEFAULT_RETURNCODE;
	
		uint iShip;
		uint iClientIDTarget = HkGetClientIDByShip(ci.dwTargetShip);
		pub::Player::GetShip(iClientID, iShip);
		uint iType;
		pub::SpaceObj::GetType(ci.dwTargetShip, iType);
		if(iType == 65536)
		{
			float fMass;
			pub::SpaceObj::GetMass(iShip, fMass);
			if(set_fSpinProtectMass!=-1.0f && !iClientIDTarget && fMass>=set_fSpinProtectMass)
			{
				Vector V1, V2;
				pub::SpaceObj::GetMotion(ci.dwTargetShip, V1, V2);
				pub::SpaceObj::GetMass(ci.dwTargetShip, fMass);
				V1.x *= set_fSpinImpulseMultiplier * fMass;
				V1.y *= set_fSpinImpulseMultiplier * fMass;
				V1.z *= set_fSpinImpulseMultiplier * fMass;
				V2.x *= set_fSpinImpulseMultiplier * fMass;
				V2.y *= set_fSpinImpulseMultiplier * fMass;
				V2.z *= set_fSpinImpulseMultiplier * fMass;
				pub::SpaceObj::AddImpulse(ci.dwTargetShip, V1, V2);
			}
		}
	}
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "SpinProtection by M0tah";
	p_PI->sShortName = "SpinProtection";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPObjCollision, PLUGIN_HkIServerImpl_SPObjCollision,0));
	return p_PI;
}