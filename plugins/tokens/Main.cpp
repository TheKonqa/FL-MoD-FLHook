#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"

PLUGIN_RETURNCODE returncode;

struct CLIENT_DATA
{
	bool bNoPvp;
};

CLIENT_DATA PluginInfo[MAX_CLIENT_ID+1];

vector<uint> set_vNoPvpGoodIDs;
map<uint, uint> set_mAffiliationTokens;

string Trim(string scIn)
{
	while (scIn.length() && (scIn[0]==' ' || scIn[0]=='	' || scIn[0]=='\n' || scIn[0]=='\r') )
		scIn = scIn.substr(1);

	while (scIn.length() && (scIn[scIn.length()-1]==L' ' || scIn[scIn.length()-1]=='	' || scIn[scIn.length()-1]=='\n' || scIn[scIn.length()-1]=='\r') )
		scIn = scIn.substr(0, scIn.length()-1);

	return scIn;
}

string GetParam(string scLine, char cSplitChar, uint iPos)
{
	uint i = 0, j = 0;
 
	string scResult = "";
	for (i = 0, j = 0; (i <= iPos) && (j < scLine.length()); j++)
	{
		if (scLine[j] == cSplitChar)
		{
			while (((j + 1) < scLine.length()) && (scLine[j+1] == cSplitChar))
				j++;

			i++;
			continue;
		}
 
		if (i == iPos)
			scResult += scLine[j];
	}
 
	return scResult;
}

void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	char szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);
	string scPluginCfgFile = string(szCurDir) + "\\flhook_plugins\\tokens.ini";

	list<INISECTIONVALUE> lstValues;
	IniGetSection(scPluginCfgFile, "NoPvpTokens", lstValues);
	set_vNoPvpGoodIDs.clear();
	foreach (lstValues, INISECTIONVALUE, it1)
	{
		set_vNoPvpGoodIDs.push_back(CreateID(Trim(GetParam(it1->scKey, ',', 0)).c_str()));
	}

	set_mAffiliationTokens.clear();
	IniGetSection(scPluginCfgFile, "AffiliationTokens", lstValues);
	foreach (lstValues, INISECTIONVALUE, it2)
	{
		string scFaction = Trim(GetParam(it2->scKey, ',', 1));
		if (!scFaction.length())
			continue;

		uint iFactionID;
		pub::Reputation::GetReputationGroup(iFactionID, scFaction.c_str());
		set_mAffiliationTokens[CreateID(Trim(GetParam(it2->scKey, ',', 0)).c_str())] = iFactionID;
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH && set_scCfgFile.length() > 0)
		LoadSettings();

	return true;
}

void ClearClientInfo(uint iClientID)
{
	returncode = DEFAULT_RETURNCODE;
	PluginInfo[iClientID].bNoPvp = false;
}

namespace HkIServerImpl
{
	void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		if (set_vNoPvpGoodIDs.size() || set_mAffiliationTokens.size())
		{
			list<CARGO_INFO> lstCargo;
			int iRemainingHoldSize;
			HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRemainingHoldSize);
			PluginInfo[iClientID].bNoPvp = false;
			foreach (lstCargo, CARGO_INFO, cargo)
			{
				if (!cargo->bMounted)
					continue;

				for (uint i=0; i < set_vNoPvpGoodIDs.size(); i++)
				{
					if (set_vNoPvpGoodIDs[i] == cargo->iArchID)
					{
						PluginInfo[iClientID].bNoPvp = true;
						break;
					}
				}
				if (PluginInfo[iClientID].bNoPvp)
					break;
			}
			
			int iRep;
			pub::Player::GetRep(iClientID, iRep);
			uint iAffil;
			Reputation::Vibe::GetAffiliation(iRep, iAffil, false);

			bool bAffiliationSet = false;
			foreach (lstCargo, CARGO_INFO, cargo)
			{
				if (cargo->bMounted && set_mAffiliationTokens.find(cargo->iArchID) != set_mAffiliationTokens.end())
				{
					if (set_mAffiliationTokens[cargo->iArchID] && iAffil != set_mAffiliationTokens[cargo->iArchID])
					{
						pub::Reputation::SetReputation(iRep, set_mAffiliationTokens[cargo->iArchID], 0.9f);
						pub::Reputation::SetAffiliation(iRep, set_mAffiliationTokens[cargo->iArchID]);
					}
					bAffiliationSet = true;
					break;
				}
			}

			if (!bAffiliationSet)
			{
				for(map<uint, uint>::iterator iter = set_mAffiliationTokens.begin(); iter!=set_mAffiliationTokens.end(); iter++)
				{
					pub::Reputation::SetReputation(iRep, iter->second, 0.0f);
					if (iAffil == iter->second)
					{
						pub::Reputation::SetAffiliation(iRep, -1);
						break;
					}
				}
			}
		}
	}

	void __stdcall SPMunitionCollision(struct SSPMunitionCollisionInfo const & ci, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		uint iClientIDTarget = HkGetClientIDByShip(ci.dwTargetShip);
		if (iClientIDTarget && (PluginInfo[iClientIDTarget].bNoPvp || PluginInfo[iClientID].bNoPvp))
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
	}

	void __stdcall SPObjCollision(struct SSPObjCollisionInfo const &ci, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;

		uint iClientIDTarget = HkGetClientIDByShip(ci.dwTargetShip);
		if (iClientIDTarget && (PluginInfo[iClientIDTarget].bNoPvp || PluginInfo[iClientID].bNoPvp))
			returncode = SKIPPLUGINS_NOFUNCTIONCALL;
	}
}

int __stdcall HkCB_MissileTorpHit(char *ECX, char *p1, DamageList *dmg)
{
	returncode = DEFAULT_RETURNCODE;
	
	char *szP;
	memcpy(&szP, ECX + 0x10, 4);
	uint iClientID;
	memcpy(&iClientID, szP + 0xB4, 4);

	if (!iClientID) 
		return 0;

	uint iInflictorShip;
	memcpy(&iInflictorShip, p1 + 4, 4);
	uint iClientIDInflictor = HkGetClientIDByShip(iInflictorShip);
	if(!iClientIDInflictor)
		return 0;

	if (PluginInfo[iClientIDInflictor].bNoPvp || PluginInfo[iClientID].bNoPvp)
	{
		returncode = SKIPPLUGINS_NOFUNCTIONCALL;
		return 1;
	}

	return 0;
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "Tokens Plugin (by Death Killer)";
	p_PI->sShortName = "tokens";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ClearClientInfo, PLUGIN_ClearClientInfo, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch, PLUGIN_HkIServerImpl_PlayerLaunch, 0));

	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPMunitionCollision, PLUGIN_HkIServerImpl_SPMunitionCollision, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPObjCollision, PLUGIN_HkIServerImpl_SPObjCollision, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCB_MissileTorpHit, PLUGIN_HkCB_MissileTorpHit, 0));
	return p_PI;
}