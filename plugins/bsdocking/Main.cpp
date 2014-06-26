#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"
#include <math.h>

#define PRINT_DISABLED() PrintUserCmdText(iClientID, L"Command disabled");

BinaryTree<MOBILE_SHIP> *set_btMobDockShipArchIDs = new BinaryTree<MOBILE_SHIP>();
BinaryTree<DOCK_RESTRICTION> *set_btJRestrict = new BinaryTree<DOCK_RESTRICTION>();

int	set_iMobileDockRadius;
int	set_iMobileDockOffset;
list<INISECTIONVALUE> lstValues;
list<MOB_UNDOCKBASEKILL> lstUndockKill;
MDOCK_DATA mdock[250];

FARPROC fpOldLaunchPos;
bool g_bInPlayerLaunch = false;
Vector g_Vlaunch;
Matrix g_Mlaunch;
bool bHasCheckedDock = false;

PLUGIN_RETURNCODE returncode;

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	string set_scCfgDockingFile;
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scCfgDockingFile = string(szCurDir) + "\\flhook_plugins\\bsdocking.ini";
    set_iMobileDockRadius = IniGetI(set_scCfgDockingFile, "General", "MobileDockRadius", -1);
	set_iMobileDockOffset = IniGetI(set_scCfgDockingFile, "General", "MobileDockOffset", -1);
	IniGetSection(set_scCfgDockingFile, "MobileBases", lstValues);
	set_btMobDockShipArchIDs->Clear();
	foreach(lstValues, INISECTIONVALUE, it7)
	{
		uint iShipArchID;
		pub::GetShipID(iShipArchID, Trim(GetParam(it7->scKey, ',', 0)).c_str());
		MOBILE_SHIP *ms = new MOBILE_SHIP(iShipArchID, ToInt(Trim(GetParam(it7->scKey, ',', 1))));
		set_btMobDockShipArchIDs->Add(ms);
	}

	IniGetSection(set_scCfgDockingFile, "DockRestrictions", lstValues);
	set_btJRestrict->Clear();
	foreach(lstValues, INISECTIONVALUE, it4)
	{
		set_btJRestrict->Add(new DOCK_RESTRICTION(CreateID(it4->scKey.c_str()), CreateID((Trim(GetParam(it4->scValue, ',', 0))).c_str()), ToInt(Trim(GetParam(it4->scValue, ',', 1))), stows(Trim(GetParamToEnd(it4->scValue, ',', 2)))));
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
	EXPORT void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID)
	{
	//Launch hook, mobile docking stuffs
	if(mdock[iClientID].bMobileDocked)
	{
		if(mdock[iClientID].lstJumpPath.size()) //Not in same system, follow jump path
		{
			uint iDockObj = mdock[iClientID].lstJumpPath.front();
			pub::SpaceObj::GetLocation(iDockObj, g_Vlaunch, g_Mlaunch);
			g_Vlaunch.x -= g_Mlaunch.data[0][2]*750;
			g_Vlaunch.y -= g_Mlaunch.data[1][2]*750;
			g_Vlaunch.z -= g_Mlaunch.data[2][2]*750;
			g_Mlaunch.data[0][0] = -g_Mlaunch.data[0][0];
			g_Mlaunch.data[1][0] = -g_Mlaunch.data[1][0];
			g_Mlaunch.data[2][0] = -g_Mlaunch.data[2][0];
			g_Mlaunch.data[0][2] = -g_Mlaunch.data[0][2];
			g_Mlaunch.data[1][2] = -g_Mlaunch.data[1][2];
			g_Mlaunch.data[2][2] = -g_Mlaunch.data[2][2];
			g_bInPlayerLaunch = true;
			Server.PlayerLaunch(iShip, iClientID);
			g_bInPlayerLaunch = false;
			if(HkDockingRestrictions(iClientID, iDockObj))
			{
				uint iShip;
				pub::Player::GetShip(iClientID, iShip);
				mdock[iClientID].bPathJump = true;
				pub::SpaceObj::InstantDock(iShip, mdock[iClientID].lstJumpPath.front(), 1);
				mdock[iClientID].lstJumpPath.pop_front();
			}
			else //Player cannot dock with object the carrier did, so place them in front of it
			{
				mdock[iClientID].lstJumpPath.clear();
			}
		}
		else if(mdock[iClientID].iDockClientID) //same system and carrier still exists
		{
			uint iDockClientID = mdock[iClientID].iDockClientID;
			uint iTargetBaseID;
			pub::Player::GetBase(iDockClientID, iTargetBaseID);
			if(iTargetBaseID) //carrier docked
			{
				uint iTargetShip = mdock[iDockClientID].iLastSpaceObjDocked;
				if(iTargetShip) //got the spaceObj of the base alright
				{
					uint iType;
					pub::SpaceObj::GetType(iTargetShip, iType);
					pub::SpaceObj::GetLocation(iTargetShip, g_Vlaunch, g_Mlaunch);
					if(iType==32) //docking ring
					{
						g_Mlaunch.data[0][0] = -g_Mlaunch.data[0][0];
						g_Mlaunch.data[1][0] = -g_Mlaunch.data[1][0];
						g_Mlaunch.data[2][0] = -g_Mlaunch.data[2][0];
						g_Mlaunch.data[0][2] = -g_Mlaunch.data[0][2];
						g_Mlaunch.data[1][2] = -g_Mlaunch.data[1][2];
						g_Mlaunch.data[2][2] = -g_Mlaunch.data[2][2];
						g_Vlaunch.x += g_Mlaunch.data[0][0]*90;
						g_Vlaunch.y += g_Mlaunch.data[1][0]*90;
						g_Vlaunch.z += g_Mlaunch.data[2][0]*90;
					}
					else
					{
						g_Vlaunch.x += g_Mlaunch.data[0][1]*set_iMobileDockOffset;
						g_Vlaunch.y += g_Mlaunch.data[1][1]*set_iMobileDockOffset;
						g_Vlaunch.z += g_Mlaunch.data[2][1]*set_iMobileDockOffset;
					}
					g_bInPlayerLaunch = true;
					Server.PlayerLaunch(iShip, iClientID);
					g_bInPlayerLaunch = false;
					pub::Player::RevertCamera(iClientID);
				}
				else //backup: set player's base to that of target, hope they won't get kicked (this shouldn't happen)
				{
					Players[iClientID].iBaseID = iTargetBaseID;
					Server.PlayerLaunch(iShip, iClientID);
				}
			}
			else //carrier not docked
			{
				uint iTargetShip;
				pub::Player::GetShip(iDockClientID, iTargetShip);
				pub::SpaceObj::GetLocation(iTargetShip, g_Vlaunch, g_Mlaunch);
				g_Vlaunch.x += g_Mlaunch.data[0][1]*set_iMobileDockOffset;
				g_Vlaunch.y += g_Mlaunch.data[1][1]*set_iMobileDockOffset;
				g_Vlaunch.z += g_Mlaunch.data[2][1]*set_iMobileDockOffset;
				g_bInPlayerLaunch = true;
				Server.PlayerLaunch(iShip, iClientID);
				g_bInPlayerLaunch = false;
				pub::Player::RevertCamera(iClientID);
			}

		}
		else //same system, carrier does not exist
		{
			g_Vlaunch = mdock[iClientID].Vlaunch;
			g_Mlaunch = mdock[iClientID].Mlaunch;
			g_bInPlayerLaunch = true;
			Server.PlayerLaunch(iShip, iClientID);
			g_bInPlayerLaunch = false;
			pub::Player::RevertCamera(iClientID);
		}
	}
	else //regular launch
	{
		Server.PlayerLaunch(iShip, iClientID);
	}
		//Mobile docking
		uint iShipArchID;
		pub::Player::GetShipID(iClientID, iShipArchID);
		MOBILE_SHIP uwFind = MOBILE_SHIP(iShipArchID);
		MOBILE_SHIP *uwFound = set_btMobDockShipArchIDs->Find(&uwFind);
		if(uwFound) //ship is mobile base
		{
			mdock[iClientID].bMobileBase = true;
			mdock[iClientID].iMaxPlayersDocked = uwFound->iMaxNumOccupants;
		}
		else
		{
			mdock[iClientID].bMobileBase = false;
		}
		if(!mdock[iClientID].iLastExitedBaseID)
		{
			mdock[iClientID].iLastExitedBaseID = 1;
		}
	}

	EXPORT void __stdcall LaunchComplete(unsigned int iBaseID, unsigned int iShip)
	{
		uint iClientID;
		iClientID = HkGetClientIDByShip(iShip);
		list<MOB_UNDOCKBASEKILL>::iterator killClient = lstUndockKill.begin();
		for(killClient = lstUndockKill.begin(); killClient!=lstUndockKill.end(); killClient++)
		{
			if(iClientID==killClient->iClientID)
			{
				Players[iClientID].iLastBaseID = killClient->iBaseID;
				if(killClient->bKill)
					HkKill(ARG_CLIENTID(iClientID));
				lstUndockKill.erase(killClient);
				return;
			}
		}

	}

	bool bIgnoreCancelMobDock = false;
	EXPORT void __stdcall BaseEnter(unsigned int iBaseID, unsigned int iClientID)
	{
		mdock[iClientID].iLastEnteredBaseID = iBaseID;
		if(!bIgnoreCancelMobDock)
		{
			if(mdock[iClientID].bMobileDocked)
			{
				wstring wscBase = ToLower(HkGetBaseNickByID(iBaseID));
				if(wscBase.find(L"mobile_proxy_base")==-1)
				{
					if(mdock[iClientID].iDockClientID)
					{
						mdock[mdock[iClientID].iDockClientID].lstPlayersDocked.remove(iClientID);
						mdock[iClientID].iDockClientID = 0;
					}
					mdock[iClientID].bMobileDocked = false;
				}
			}
		}
		else
		bIgnoreCancelMobDock = false;
	}

	EXPORT void __stdcall DisConnect(unsigned int iClientID, enum EFLConnection p2)
	{
		Vector VCharFilePos;
	    wstring wscPlayerName = L"", wscSystem = L"";
		uint iShip;
		pub::Player::GetShip(iClientID, iShip);
		//mobile docking
		try
		{
		if(mdock[iClientID].bMobileDocked)
		{
			if(!iShip) //Docked at carrier
			{
				if(mdock[iClientID].iDockClientID) //carrier still exists
				{
					Players[iClientID].iLastBaseID = Players[mdock[iClientID].iDockClientID].iLastBaseID;
					uint iTargetShip;
					pub::Player::GetShip(mdock[iClientID].iDockClientID, iTargetShip);
					if(iTargetShip) //carrier in space
					{
						Matrix m;
						pub::SpaceObj::GetLocation(iTargetShip, VCharFilePos, m);
						wscPlayerName = Players.GetActiveCharacterName(iClientID);
					}
					else //carrier docked
					{
						Players[iClientID].iBaseID = Players[mdock[iClientID].iDockClientID].iBaseID;
					}
					wscSystem = HkGetPlayerSystem(iClientID);
					//remove from docked client list
					mdock[mdock[iClientID].iDockClientID].lstPlayersDocked.remove(iClientID);
				}
				else //carrier does not exist, use stored location info
				{
					if(mdock[iClientID].lstJumpPath.size())
					{
						uint iSystemID;
						pub::GetSystemGateConnection(mdock[iClientID].lstJumpPath.back(), iSystemID);
						wscSystem = HkGetSystemNickByID(iSystemID);
					}
					list<MOB_UNDOCKBASEKILL>::iterator killClient = lstUndockKill.begin(); //Find the last_base
					for(killClient = lstUndockKill.begin(); killClient!=lstUndockKill.end(); killClient++)
					{
						if(iClientID==killClient->iClientID)
						{
							Players[iClientID].iBaseID = killClient->iBaseID;
							Players[iClientID].iLastBaseID = killClient->iBaseID;
							lstUndockKill.erase(killClient);
							break;
						}
					}
					VCharFilePos = mdock[iClientID].Vlaunch;
					wscPlayerName = Players.GetActiveCharacterName(iClientID);
				}
			}
			else //in space, update last base only
			{
				if(mdock[iClientID].iDockClientID) //carrier still exists
				{
					Players[iClientID].iLastBaseID = Players[mdock[iClientID].iDockClientID].iLastBaseID;
				}
			}
		}
		if(mdock[iClientID].lstPlayersDocked.size())
		{
			Matrix m;
			Vector v;
			uint iShip;
			pub::Player::GetShip(iClientID, iShip);
			if(iShip)
				pub::SpaceObj::GetLocation(iShip, v, m);
			foreach(mdock[iClientID].lstPlayersDocked, uint, dockedClientID) //go through all of the docked players and deal with them
			{
				uint iDockedShip;
				pub::Player::GetShip(*dockedClientID, iDockedShip);
				if(iDockedShip) //player is in space
				{
					Players[*dockedClientID].iLastBaseID = iShip ? Players[iClientID].iLastBaseID : Players[iClientID].iBaseID;
					if(!mdock[*dockedClientID].lstJumpPath.size())
					{
						mdock[*dockedClientID].bMobileDocked = false;
					}
				}
				else //player is docked
				{
					if(iShip) //carrier is in space
					{
						mdock[*dockedClientID].Vlaunch = v;
						mdock[*dockedClientID].Mlaunch = m;
						MOB_UNDOCKBASEKILL dKill;
						dKill.iClientID = *dockedClientID;
						dKill.iBaseID = Players[iClientID].iLastBaseID;
						dKill.bKill = false;
						lstUndockKill.push_back(dKill);
					}
					else //carrier is docked
					{
						uint iTargetShip = mdock[iClientID].iLastSpaceObjDocked;
						MOB_UNDOCKBASEKILL dKill;
						dKill.iClientID = *dockedClientID;
						dKill.iBaseID = mdock[iClientID].iLastEnteredBaseID;
						dKill.bKill = false;
						lstUndockKill.push_back(dKill);
						if(iTargetShip) //got the spaceObj of the base alright
						{
							uint iType;
							pub::SpaceObj::GetType(iTargetShip, iType);
							pub::SpaceObj::GetLocation(iTargetShip, mdock[*dockedClientID].Vlaunch, mdock[*dockedClientID].Mlaunch);
							if(iType==32)
							{
								mdock[*dockedClientID].Mlaunch.data[0][0] = -mdock[*dockedClientID].Mlaunch.data[0][0];
								mdock[*dockedClientID].Mlaunch.data[1][0] = -mdock[*dockedClientID].Mlaunch.data[1][0];
								mdock[*dockedClientID].Mlaunch.data[2][0] = -mdock[*dockedClientID].Mlaunch.data[2][0];
								mdock[*dockedClientID].Mlaunch.data[0][2] = -mdock[*dockedClientID].Mlaunch.data[0][2];
								mdock[*dockedClientID].Mlaunch.data[1][2] = -mdock[*dockedClientID].Mlaunch.data[1][2];
								mdock[*dockedClientID].Mlaunch.data[2][2] = -mdock[*dockedClientID].Mlaunch.data[2][2];
								mdock[*dockedClientID].Vlaunch.x += mdock[*dockedClientID].Mlaunch.data[0][0]*90;
								mdock[*dockedClientID].Vlaunch.y += mdock[*dockedClientID].Mlaunch.data[1][0]*90;
								mdock[*dockedClientID].Vlaunch.z += mdock[*dockedClientID].Mlaunch.data[2][0]*90;
							}
							else
							{
								mdock[*dockedClientID].Vlaunch.x += mdock[*dockedClientID].Mlaunch.data[0][1]*set_iMobileDockOffset;
								mdock[*dockedClientID].Vlaunch.y += mdock[*dockedClientID].Mlaunch.data[1][1]*set_iMobileDockOffset;
								mdock[*dockedClientID].Vlaunch.z += mdock[*dockedClientID].Mlaunch.data[2][1]*set_iMobileDockOffset;
							}
						}
						else //backup: set player's base to that of target, hope they won't get kicked (this shouldn't happen)
						{
							Players[*dockedClientID].iBaseID = Players[iClientID].iBaseID;
						}
					}
				}
				mdock[*dockedClientID].iDockClientID = 0;
			}
			mdock[iClientID].lstPlayersDocked.clear();

		}

		foreach(lstUndockKill, MOB_UNDOCKBASEKILL, bkill)
		{
			if(bkill->iClientID == iClientID)
			{
				lstUndockKill.erase(bkill);
				break;
			}
		}

		}catch(...) { AddLog("Exception in %s", __FUNCTION__); }
		Server.DisConnect(iClientID, p2);
		try
		{
		//mobile docking - add position, system to char file
		if(wscPlayerName.length())
		{
			list<wstring> lstCharFile;
			if(HKHKSUCCESS(HkReadCharFile(wscPlayerName, lstCharFile)))
			{
				wchar_t wszPos[32];
				swprintf(wszPos, L"pos = %f, %f, %f", VCharFilePos.x, VCharFilePos.y, VCharFilePos.z);
				list<wstring>::iterator line = lstCharFile.begin();
				wstring wscCharFile = L"";
				bool bReplacedBase = false, bFoundPos = false, bFoundSystem = wscSystem.length() ? false : true;
				for(line = lstCharFile.begin(); line!=lstCharFile.end(); line++)
				{
					wstring wscNewLine = *line;
					if(!bReplacedBase && line->find(L"base")==0)
					{
						wscNewLine = L"last_" + *line;
						bReplacedBase = true;
						continue; //for now
					}
					if(!bFoundPos && line->find(L"system")==0)
					{
						if(!bFoundSystem)
						{
							wscNewLine = L"system = " + wscSystem;
							bFoundSystem = true;
						}
						wscNewLine += L"\n";
						wscNewLine += wszPos;
						bFoundPos = true;
					}
					wscCharFile += wscNewLine + L"\n";
				}
				wscCharFile.substr(0, wscCharFile.length()-1);
				HkWriteCharFile(wscPlayerName, wscCharFile);
			}
		}

	} catch(...) { AddLog("Exception in %s", __FUNCTION__); }
	}
		
	EXPORT void __stdcall CharacterInfoReq(unsigned int iClientID, bool p2)
	{
		Vector VCharFilePos;
		wstring wscPlayerName = L"", wscSystem = L"";
		try
		{
			uint iShip = 0;
			pub::Player::GetShip(iClientID, iShip);

			if(!mdock[iClientID].tmCharInfoReqAfterDeath)
			{
				if(mdock[iClientID].iControllerID)
				{
					ConPrint(L"DestroyFLHook %u\n", mdock[iClientID].iControllerID);
					pub::Controller::Destroy(mdock[iClientID].iControllerID);
					mdock[iClientID].iControllerID = 0;
				}

				//mobile docking
				if(mdock[iClientID].bMobileDocked)
				{
					if(!iShip) //Docked at carrier
					{
						if(mdock[iClientID].iDockClientID) //carrier still exists
						{
							Players[iClientID].iLastBaseID = Players[mdock[iClientID].iDockClientID].iLastBaseID;
							uint iTargetShip;
							pub::Player::GetShip(mdock[iClientID].iDockClientID, iTargetShip);
							if(iTargetShip) //carrier in space
							{
								Matrix m;
								pub::SpaceObj::GetLocation(iTargetShip, VCharFilePos, m);
								wscPlayerName = Players.GetActiveCharacterName(iClientID);
							}
							else //carrier docked
							{
								Players[iClientID].iBaseID = Players[mdock[iClientID].iDockClientID].iBaseID;
							}
							wscSystem = HkGetPlayerSystem(iClientID);
							//remove from docked client list
							mdock[mdock[iClientID].iDockClientID].lstPlayersDocked.remove(iClientID);
						}
						else //carrier does not exist, use stored location info
						{
							if(mdock[iClientID].lstJumpPath.size())
							{
								uint iSystemID;
								pub::GetSystemGateConnection(mdock[iClientID].lstJumpPath.back(), iSystemID);
								wscSystem = HkGetSystemNickByID(iSystemID);
							}
							list<MOB_UNDOCKBASEKILL>::iterator killClient = lstUndockKill.begin(); //Find the last_base
							for(killClient = lstUndockKill.begin(); killClient!=lstUndockKill.end(); killClient++)
							{
								if(iClientID==killClient->iClientID)
								{
									Players[iClientID].iBaseID = killClient->iBaseID;
									Players[iClientID].iLastBaseID = killClient->iBaseID;
									lstUndockKill.erase(killClient);
									break;
								}
							}
							VCharFilePos = mdock[iClientID].Vlaunch;
							wscPlayerName = Players.GetActiveCharacterName(iClientID);
						}
					}
					else //in space, update last base only
					{
						if(mdock[iClientID].iDockClientID) //carrier still exists
						{
							Players[iClientID].iLastBaseID = Players[mdock[iClientID].iDockClientID].iLastBaseID;
						}
					}
				}
				if(mdock[iClientID].lstPlayersDocked.size())
				{
					Matrix m;
					Vector v;
					uint iShip;
					pub::Player::GetShip(iClientID, iShip);
					if(iShip)
						pub::SpaceObj::GetLocation(iShip, v, m);
					foreach(mdock[iClientID].lstPlayersDocked, uint, dockedClientID) //go through all of the docked players and deal with them
					{
						uint iDockedShip;
						pub::Player::GetShip(*dockedClientID, iDockedShip);
						if(iDockedShip) //player is in space
						{
							Players[*dockedClientID].iLastBaseID = iShip ? Players[iClientID].iLastBaseID : Players[iClientID].iBaseID;
							if(!mdock[*dockedClientID].lstJumpPath.size())
							{
								mdock[*dockedClientID].bMobileDocked = false;
							}
						}
						else //player is docked
						{
							if(iShip) //carrier is in space
							{
								mdock[*dockedClientID].Vlaunch = v;
								mdock[*dockedClientID].Mlaunch = m;
								MOB_UNDOCKBASEKILL dKill;
								dKill.iClientID = *dockedClientID;
								dKill.iBaseID = Players[iClientID].iLastBaseID;
								dKill.bKill = false;
								lstUndockKill.push_back(dKill);
							}
							else //carrier is docked
							{
								uint iTargetShip = mdock[iClientID].iLastSpaceObjDocked;
								MOB_UNDOCKBASEKILL dKill;
								dKill.iClientID = *dockedClientID;
								dKill.iBaseID = mdock[iClientID].iLastEnteredBaseID;
								dKill.bKill = false;
								lstUndockKill.push_back(dKill);
								if(iTargetShip) //got the spaceObj of the base alright
								{
									uint iType;
									pub::SpaceObj::GetType(iTargetShip, iType);
									pub::SpaceObj::GetLocation(iTargetShip, mdock[*dockedClientID].Vlaunch, mdock[*dockedClientID].Mlaunch);
									if(iType==32)
									{
										mdock[*dockedClientID].Mlaunch.data[0][0] = -mdock[*dockedClientID].Mlaunch.data[0][0];
										mdock[*dockedClientID].Mlaunch.data[1][0] = -mdock[*dockedClientID].Mlaunch.data[1][0];
										mdock[*dockedClientID].Mlaunch.data[2][0] = -mdock[*dockedClientID].Mlaunch.data[2][0];
										mdock[*dockedClientID].Mlaunch.data[0][2] = -mdock[*dockedClientID].Mlaunch.data[0][2];
										mdock[*dockedClientID].Mlaunch.data[1][2] = -mdock[*dockedClientID].Mlaunch.data[1][2];
										mdock[*dockedClientID].Mlaunch.data[2][2] = -mdock[*dockedClientID].Mlaunch.data[2][2];
										mdock[*dockedClientID].Vlaunch.x += mdock[*dockedClientID].Mlaunch.data[0][0]*90;
										mdock[*dockedClientID].Vlaunch.y += mdock[*dockedClientID].Mlaunch.data[1][0]*90;
										mdock[*dockedClientID].Vlaunch.z += mdock[*dockedClientID].Mlaunch.data[2][0]*90;
									}
									else
									{
										mdock[*dockedClientID].Vlaunch.x += mdock[*dockedClientID].Mlaunch.data[0][1]*set_iMobileDockOffset;
										mdock[*dockedClientID].Vlaunch.y += mdock[*dockedClientID].Mlaunch.data[1][1]*set_iMobileDockOffset;
										mdock[*dockedClientID].Vlaunch.z += mdock[*dockedClientID].Mlaunch.data[2][1]*set_iMobileDockOffset;
									}
								}
								else //backup: set player's base to that of target, hope they won't get kicked (this shouldn't happen)
								{
									Players[*dockedClientID].iBaseID = Players[iClientID].iBaseID;
								}
							}
						}
						mdock[*dockedClientID].iDockClientID = 0;
					}
					mdock[iClientID].lstPlayersDocked.clear();
				}
			}
			else //Player respawning
			{
				/*mstime tmCall = mdock[iClientID].tmCharInfoReqAfterDeath + set_iRespawnDelay;
				if(tmCall > timeInMS())
				{
					lstRespawnDelay.push_back(RESPAWN_DELAY(iClientID, p2, tmCall));
					return;
				}
				mdock[iClientID].tmCharInfoReqAfterDeath = 0;*/
			}

			mdock[iClientID].iControllerID = 0;

			if(iShip)
			{ // in space
				ClientInfo[iClientID].tmF1Time = timeInMS() + set_iAntiF1;
				return;
			}

		} 
		catch(...) { AddLog("Exception in %s", __FUNCTION__); }
	
	try{
		//mobile docking - add position to char file
		if(wscPlayerName.length())
		{
			list<wstring> lstCharFile;
			if(HKHKSUCCESS(HkReadCharFile(wscPlayerName, lstCharFile)))
			{
				wchar_t wszPos[32];
				swprintf(wszPos, L"pos = %f, %f, %f", VCharFilePos.x, VCharFilePos.y, VCharFilePos.z);
				list<wstring>::iterator line = lstCharFile.begin();
				wstring wscCharFile = L"";
				bool bReplacedBase = false, bFoundPos = false, bFoundSystem = wscSystem.length() ? false : true;
				for(line = lstCharFile.begin(); line!=lstCharFile.end(); line++)
				{
					wstring wscNewLine = *line;
					if(!bReplacedBase && line->find(L"base")==0)
					{
						wscNewLine = L"last_" + *line;
						bReplacedBase = true;
						continue; //for now
					}
					if(!bFoundPos && line->find(L"system")==0)
					{
						if(!bFoundSystem)
						{
							wscNewLine = L"system = " + wscSystem;
							bFoundSystem = true;
						}
						wscNewLine += L"\n";
						wscNewLine += wszPos;
						bFoundPos = true;
					}
					wscCharFile += wscNewLine + L"\n";
				}
				wscCharFile.substr(0, wscCharFile.length()-1);
				HkWriteCharFile(wscPlayerName, wscCharFile);
			}
		}
	} catch(...) { AddLog("Exception in %s", __FUNCTION__); }
	}

	EXPORT void __stdcall JumpInComplete(unsigned int iSystemID, unsigned int iShip)
	{
		uint iClientID = HkGetClientIDByShip(iShip);
		if(!iClientID)
			return;
		if(mdock[iClientID].lstPlayersDocked.size())
		{
			string scBase = HkGetPlayerSystemS(iClientID) + "_Mobile_Proxy_Base";
			uint iBaseID = 0;
			pub::GetBaseID(iBaseID, (scBase).c_str());
			if(iBaseID)
			{
				foreach(mdock[iClientID].lstPlayersDocked, uint, dockedClientID)
				{
					Players[*dockedClientID].iLastBaseID = iBaseID;
					for(list<uint>::iterator jumpObj = mdock[*dockedClientID].lstJumpPath.begin(); jumpObj!=mdock[*dockedClientID].lstJumpPath.end(); jumpObj++)
					{
						uint iJumpSystemID;
						pub::SpaceObj::GetSystem(*jumpObj, iJumpSystemID);
						if(iJumpSystemID==iSystemID)
						{
							mdock[*dockedClientID].lstJumpPath.erase(jumpObj, mdock[*dockedClientID].lstJumpPath.end());
							break;
						}
					}
				}
			}
		}
		// mobile docking switching systems
		if(mdock[iClientID].lstJumpPath.size()) //player is traveling to carrier
		{
			mdock[iClientID].bPathJump = true;
			uint iDockObj = mdock[iClientID].lstJumpPath.front();
			if(HkDockingRestrictions(iClientID, iDockObj))
			{
				pub::SpaceObj::InstantDock(iShip, iDockObj, 1);
				mdock[iClientID].lstJumpPath.pop_front();
				return;
			}
			else //Player cannot dock with object the carrier did, so place them in front of it
			{
				mdock[iClientID].lstJumpPath.clear();
				Vector vBeam;
				Matrix mBeam;
				pub::SpaceObj::GetLocation(iDockObj, vBeam, mBeam);
				vBeam.x -= mBeam.data[0][2]*750;
				vBeam.y -= mBeam.data[1][2]*750;
				vBeam.z -= mBeam.data[2][2]*750;
				mBeam.data[0][0] = -mBeam.data[0][0];
				mBeam.data[1][0] = -mBeam.data[1][0];
				mBeam.data[2][0] = -mBeam.data[2][0];
				mBeam.data[0][2] = -mBeam.data[0][2];
				mBeam.data[1][2] = -mBeam.data[1][2];
				mBeam.data[2][2] = -mBeam.data[2][2];
				HkBeamInSys(ARG_CLIENTID(iClientID), vBeam, mBeam);
			}
		}
		else
		{
			if(mdock[iClientID].bPathJump)
			{
				if(mdock[iClientID].iDockClientID) //carrier still exists
				{
					uint iTargetShip;
					pub::Player::GetShip(mdock[iClientID].iDockClientID, iTargetShip);
					if(iTargetShip) //carrier in space
					{
						Vector vBeam;
						Matrix mBeam;
						pub::SpaceObj::GetLocation(iTargetShip, vBeam, mBeam);
						vBeam.x += mBeam.data[0][1]*set_iMobileDockOffset;
						vBeam.y += mBeam.data[1][1]*set_iMobileDockOffset;
						vBeam.z += mBeam.data[2][1]*set_iMobileDockOffset;
						HkBeamInSys(ARG_CLIENTID(iClientID), vBeam, mBeam);
					}
					else //carrier docked
					{
						uint iTargetShip = mdock[mdock[iClientID].iDockClientID].iLastSpaceObjDocked;
						if(iTargetShip) //got the spaceObj of the base alright
						{
							uint iType;
							pub::SpaceObj::GetType(iTargetShip, iType);
							Vector vBeam;
							Matrix mBeam;
							pub::SpaceObj::GetLocation(iTargetShip, vBeam, mBeam);
							if(iType==32)
							{
								mBeam.data[0][0] = -mBeam.data[0][0];
								mBeam.data[1][0] = -mBeam.data[1][0];
								mBeam.data[2][0] = -mBeam.data[2][0];
								mBeam.data[0][2] = -mBeam.data[0][2];
								mBeam.data[1][2] = -mBeam.data[1][2];
								mBeam.data[2][2] = -mBeam.data[2][2];
								vBeam.x += mBeam.data[0][0]*90;
								vBeam.y += mBeam.data[1][0]*90;
								vBeam.z += mBeam.data[2][0]*90;
							}
							else
							{
								vBeam.x += mBeam.data[0][1]*set_iMobileDockOffset;
								vBeam.y += mBeam.data[1][1]*set_iMobileDockOffset;
								vBeam.z += mBeam.data[2][1]*set_iMobileDockOffset;
							}
							HkBeamInSys(ARG_CLIENTID(iClientID), vBeam, mBeam);
						}
					}
				}
				else //carrier does not exist, use stored info
				{
					HkBeamInSys(ARG_CLIENTID(iClientID), mdock[iClientID].Vlaunch, mdock[iClientID].Mlaunch);
					mdock[iClientID].bMobileDocked = false;
				}
				mdock[iClientID].bPathJump = false;
			}
		}
	}

	EXPORT void __stdcall SystemSwitchOutComplete(unsigned int iShip, unsigned int iClientID)
	{
		if(mdock[iClientID].lstPlayersDocked.size())
		{
			uint iTarget;
			pub::SpaceObj::GetTarget(iShip, iTarget);
			foreach(mdock[iClientID].lstPlayersDocked, uint, dockedClientID)
			{
				uint iDockedShip;
				pub::Player::GetShip(*dockedClientID, iDockedShip);
				if(!iDockedShip)
					mdock[*dockedClientID].lstJumpPath.push_back(iTarget);
			}
		}

	}

	EXPORT void __stdcall RequestEvent(int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned long p5, unsigned int p6)
	{
		if(!p1) //docking
		{
			DOCK_RESTRICTION drFind = DOCK_RESTRICTION(p3);
			DOCK_RESTRICTION *drFound = set_btJRestrict->Find(&drFind);
			if(drFound)
			{
				list<CARGO_INFO> lstCargo;
				int iSpaceRemaining;
				bool bPresent = false;
				HkEnumCargo((wchar_t*)Players.GetActiveCharacterName(p6), lstCargo, iSpaceRemaining);
				foreach(lstCargo, CARGO_INFO, cargo)
				{
					if(cargo->iArchID == drFound->iArchID)//Item is present
					{
						bPresent = true;
						break;
					}
				}
				if(!bPresent)
				{
					pub::Player::SendNNMessage(p6, pub::GetNicknameId("info_access_denied"));
					PrintUserCmdText(p6, drFound->wscDeniedMsg);
					returncode = NOFUNCTIONCALL;
					return; //block dock
				}
			}
			bHasCheckedDock = true;
		}
		returncode = DEFAULT_RETURNCODE;

		//mobile docking
			if(!((uint)(p3/1000000000))) //Docking at non solar
			{
				if(mdock[p6].bMobileBase) //prevent mobile bases from docking with each other
				{
					pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
					return;
				}
				uint iTargetClientID = HkGetClientIDByShip(p3);
				if(iTargetClientID)
				{
					if(!mdock[iTargetClientID].bMobileBase)
					{
						pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
						return;
					}
					//Make sure players are in same group
					uint iGroupID = Players.GetGroupID(p6), iTargetGroupID = Players.GetGroupID(iTargetClientID);
					if(!iGroupID || !iTargetGroupID || iGroupID!=iTargetGroupID)
					{
						pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
						return;
					}
					//Dock player at mobile dock base in system
					Matrix mClient, mTarget;
					Vector vClient, vTarget;
					pub::SpaceObj::GetLocation(p2, vClient, mClient);
					pub::SpaceObj::GetLocation(p3, vTarget, mTarget);
					uint iDunno;
					IObjInspectImpl *inspect;
					GetShipInspect(p3, inspect, iDunno);
					if(HkDistance3D(vClient, vTarget) > set_iMobileDockRadius+inspect->cobject()->hierarchy_radius()) //outside of docking radius
					{
						pub::Player::SendNNMessage(p6, pub::GetNicknameId("nnv_dock_too_far"));
						return;
					}
					if(!HkIsOkayToDock(p6, iTargetClientID))
					{
						pub::Player::SendNNMessage(p6, pub::GetNicknameId("insufficient_cargo_space"));
						return;
					}
					string scSystem = HkGetPlayerSystemS(p6);
					string scBase = scSystem + "_Mobile_Proxy_Base";
					uint iBaseID;
					if(pub::GetBaseID(iBaseID, (scBase).c_str()) == -4) //base does not exist
					{
						ConPrint(L"WARNING: Mobile docking proxy base does not exist in system " + stows(scSystem) + L"\n");
						pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
						return;
					}
					uint iTargetProxyObj = CreateID(scBase.c_str());
					bIgnoreCancelMobDock = true;
					pub::SpaceObj::InstantDock(p2, iTargetProxyObj, 1);
					mdock[iTargetClientID].lstPlayersDocked.remove(p6);
					mdock[iTargetClientID].lstPlayersDocked.push_back(p6);
					mdock[p6].bMobileDocked = true;
					mdock[p6].iDockClientID = iTargetClientID;
					return;
				}
				else
				{
					pub::Player::SendNNMessage(p6, pub::GetNicknameId("dock_disallowed"));
					return;
				}
			} 
			if(mdock[p6].bMobileBase)
			{
				mdock[p6].iLastSpaceObjDocked = p3;
			}
			mdock[p6].bCheckedDock = true;
		    if(!HkDockingRestrictions(p6, p3))
			return;
	}

	EXPORT int __stdcall Update()
	{
		returncode = DEFAULT_RETURNCODE;
		static bool bFirstTime = true;
		if(bFirstTime) 
		{
			bFirstTime = false;
			// check for logged in players and reset their connection data
			struct PlayerData *pPD = 0; 
			while(pPD = Players.traverse_active(pPD))
			ClearDOCKData(HkGetClientIdFromPD(pPD));
		}
		return 0;
	}
}

EXPORT int __cdecl HkCb_Dock_Call(unsigned int const &iShip, unsigned int const &iDockTarget, int iCancel, enum DOCK_HOST_RESPONSE response)
{
	if(!iCancel)
	{
		if(!bHasCheckedDock)
		{
			uint iClientID = HkGetClientIDByShip(iShip);
			if(iClientID)
			{
				DOCK_RESTRICTION drFind = DOCK_RESTRICTION(iDockTarget);
				DOCK_RESTRICTION *drFound = set_btJRestrict->Find(&drFind);
				if(drFound)
				{
					list<CARGO_INFO> lstCargo;
					int iSpaceRemaining;
					bool bPresent = false;
					HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iSpaceRemaining);
					foreach(lstCargo, CARGO_INFO, cargo)
					{
						if(cargo->iArchID==drFound->iArchID)//Item is present
						{
							bPresent = true;
							break;
						}
					}
					if(!bPresent)
					{
						pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("info_access_denied"));
						PrintUserCmdText(iClientID, drFound->wscDeniedMsg);
						returncode = NOFUNCTIONCALL;
						return 0; //block dock
					}
				}
			}
		}
	}
	bHasCheckedDock = false;

	returncode = DEFAULT_RETURNCODE;
	return 0;
}

EXPORT void __stdcall ShipDestroyed(DamageList *_dmg, char *szECX, uint iKill)
	{
		char *szP;
		memcpy(&szP, szECX + 0x10, 4);
		uint iClientID;
		memcpy(&iClientID, szP + 0xB4, 4);
		if(iKill)
		{
			if(iClientID) { // a player was killed
				//mobile docking - clear list of docked players, set them to die
				if(mdock[iClientID].lstPlayersDocked.size())
				{
					uint iBaseID = Players[iClientID].iLastBaseID;
					foreach(mdock[iClientID].lstPlayersDocked, uint, dockedClientID)
					{
						mdock[*dockedClientID].bMobileDocked = false;
						mdock[*dockedClientID].iDockClientID = 0;
						mdock[*dockedClientID].lstJumpPath.clear();
						uint iDockedShip;
						pub::Player::GetShip(*dockedClientID, iDockedShip);
						if(iDockedShip) //docked player is in space
						{
							Players[*dockedClientID].iLastBaseID = iBaseID;
						}
						else
						{
							MOB_UNDOCKBASEKILL dKill;
							dKill.iClientID = *dockedClientID;
							dKill.iBaseID = iBaseID;
							dKill.bKill = true;
							lstUndockKill.push_back(dKill);
						}
					}
					mdock[iClientID].lstPlayersDocked.clear();
				}
				//mobile docking - update docked base to one in current system
				if(mdock[iClientID].bMobileDocked)
				{
					if(mdock[iClientID].iDockClientID)
					{
						uint iBaseID;
						pub::GetBaseID(iBaseID, (HkGetPlayerSystemS(mdock[iClientID].iDockClientID) + "_Mobile_Proxy_Base").c_str());
						Players[iClientID].iLastBaseID = iBaseID;
					}
				}
				mdock[iClientID].tmCharInfoReqAfterDeath = timeInMS();
			}
		}
	}

float HkDistance3D(Vector v1, Vector v2)
{
	float sq1 = v1.x-v2.x, sq2 = v1.y-v2.y, sq3 = v1.z-v2.z;
	return sqrt( sq1*sq1 + sq2*sq2 + sq3*sq3 );
}

bool HkIsOkayToDock(uint iClientID, uint iTargetClientID)
{
	if(mdock[iTargetClientID].iMaxPlayersDocked == -1)
		return true;

	uint iNumDocked = 0;
	foreach(mdock[iTargetClientID].lstPlayersDocked, uint, iDockedClientID)
	{
		uint iShip;
		pub::Player::GetShip(*iDockedClientID, iShip);
		if(!iShip)
			iNumDocked++;
	}
	if(iNumDocked >= (uint)mdock[iTargetClientID].iMaxPlayersDocked)
		return false;

	return true;
}

HK_ERROR HkBeamInSys(wstring wscCharname, Vector vOffsetVector, Matrix mOrientation)
{
	HK_GET_CLIENTID(iClientID, wscCharname);

	// check if logged in
	if(iClientID == -1)
		return HKE_PLAYER_NOT_LOGGED_IN;

	uint iShip, iSystemID;
	pub::Player::GetShip(iClientID, iShip);
	pub::Player::GetSystem(iClientID, iSystemID);

	Quaternion qRotation = HkMatrixToQuaternion(mOrientation);
	
	LAUNCH_PACKET* ltest = new LAUNCH_PACKET;
	ltest->iShip = iShip;
	ltest->iDunno[0] = 0;
	ltest->iDunno[1] = 0xFFFFFFFF;
	ltest->fRotate[0] = qRotation.w;
	ltest->fRotate[1] = qRotation.x;
	ltest->fRotate[2] = qRotation.y;
	ltest->fRotate[3] = qRotation.z;
	ltest->fPos[0] = vOffsetVector.x;
	ltest->fPos[1] = vOffsetVector.y;
	ltest->fPos[2] = vOffsetVector.z;

	char* ClientOffset = (char*)hModRemoteClient + ADDR_RMCLIENT_CLIENT;
	char* SendLaunch = (char*)hModRemoteClient + ADDR_RMCLIENT_LAUNCH;

	__asm {
		mov ecx, ClientOffset
		mov ecx, [ecx]
		push [ltest]
		push [iClientID]
		call SendLaunch
	}

	pub::SpaceObj::Relocate(iShip, iSystemID, vOffsetVector, mOrientation);

	delete ltest;

	return HKE_OK;
}

Quaternion HkMatrixToQuaternion(Matrix m)
{
	Quaternion quaternion;
	quaternion.w = sqrt( max( 0, 1 + m.data[0][0] + m.data[1][1] + m.data[2][2] ) ) / 2; 
	quaternion.x = sqrt( max( 0, 1 + m.data[0][0] - m.data[1][1] - m.data[2][2] ) ) / 2; 
	quaternion.y = sqrt( max( 0, 1 - m.data[0][0] + m.data[1][1] - m.data[2][2] ) ) / 2; 
	quaternion.z = sqrt( max( 0, 1 - m.data[0][0] - m.data[1][1] + m.data[2][2] ) ) / 2; 
	quaternion.x = (float)_copysign( quaternion.x, m.data[2][1] - m.data[1][2] );
	quaternion.y = (float)_copysign( quaternion.y, m.data[0][2] - m.data[2][0] );
	quaternion.z = (float)_copysign( quaternion.z, m.data[1][0] - m.data[0][1] );

	return quaternion;
}

void UserCmd_Dock(uint iClientID, const wstring &wscParam)
{
	if(set_iMobileDockRadius == -1)
	{
		PRINT_DISABLED();
		return;
	}
	wstring wscError[] = 
	{
		L"Error: Could not dock",
		L"Usage: /dock",
	};

	uint iShip, iDockTarget;
	pub::Player::GetShip(iClientID, iShip);
	if(!iShip)
	{
		PrintUserCmdText(iClientID, L"Error: you are already docked");
		return;
	}
	pub::SpaceObj::GetTarget(iShip, iDockTarget);
	if(((uint)(iDockTarget/1000000000))) //trying to dock at solar
	{
		PrintUserCmdText(iClientID, L"Error: you must use /dock on players");
		return;
	}
	pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("dock"));
	HkIServerImpl::RequestEvent(0, iShip, iDockTarget, 0, 0, iClientID);
}

bool HkDockingRestrictions(uint iClientID, uint iDockObj)
{
	mdock[iClientID].lstRemCargo.clear();
	DOCK_RESTRICTION jrFind = DOCK_RESTRICTION(iDockObj);
	DOCK_RESTRICTION *jrFound = set_btJRestrict->Find(&jrFind);
	if(jrFound)
	{
		list<CARGO_INFO> lstCargo;
		bool bPresent = false;
		int iRem;
		HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
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
						mdock[iClientID].lstRemCargo.push_back(cm);
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

void ClearDOCKData(uint iClientID)
{
	mdock[iClientID].bMobileDocked = false;
	mdock[iClientID].iDockClientID = 0;
	mdock[iClientID].lstJumpPath.clear();
	mdock[iClientID].bPathJump = false;
	mdock[iClientID].tmCharInfoReqAfterDeath = 0;
	mdock[iClientID].Vlaunch.x = 0;
	mdock[iClientID].Vlaunch.y = 0;
	mdock[iClientID].Vlaunch.z = 0;
	for(uint i=0; i<3; i++)
	{
		for(uint j=0; j<3; j++)
		{
			mdock[iClientID].Mlaunch.data[i][j] = 0.0f;
		}
	}
	mdock[iClientID].bMobileBase = false;
	mdock[iClientID].lstPlayersDocked.clear();
	mdock[iClientID].iLastSpaceObjDocked = 0;
	mdock[iClientID].iLastEnteredBaseID = 0;
	mdock[iClientID].lstRemCargo.clear();
	mdock[iClientID].iControllerID = 0;
	mdock[iClientID].bCheckedDock = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string HkGetPlayerSystemS(uint iClientID)
{
	uint iSystemID;
	pub::Player::GetSystem(iClientID, iSystemID);
	char szSystemname[1024] = "";
	pub::GetSystemNickname(szSystemname, sizeof(szSystemname), iSystemID);
	return szSystemname;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EXPORT void UserCmd_Help(uint iClientID, const wstring &wscParam)
{
    PrintUserCmdText(iClientID, L"/d");
	PrintUserCmdText(iClientID, L"/dock");
}

typedef void (*_UserCmdProc)(uint, const wstring &);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
};

USERCMD UserCmds[] =
{
	{ L"/d",				UserCmd_Dock},
	{ L"/dock",			    UserCmd_Dock},
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
wstring Trim(wstring wscIn)
{
	while(wscIn.length() && (wscIn[0]==L' ' || wscIn[0]==L'	' || wscIn[0]==L'\n' || wscIn[0]==L'\r') )
	{
		wscIn = wscIn.substr(1);
	}
	while(wscIn.length() && (wscIn[wscIn.length()-1]==L' ' || wscIn[wscIn.length()-1]==L'	' || wscIn[wscIn.length()-1]==L'\n' || wscIn[wscIn.length()-1]==L'\r') )
	{
		wscIn = wscIn.substr(0, wscIn.length()-1);
	}
	return wscIn;
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

wstring GetParamToEnd(wstring wscLine, wchar_t wcSplitChar, uint iPos)
{
	for(uint i = 0, iCurArg = 0; (i < wscLine.length()); i++)
	{
		if(wscLine[i] == wcSplitChar)
		{
			iCurArg++;

			if(iCurArg == iPos)
				return wscLine.substr(i + 1);

			while(((i + 1) < wscLine.length()) && (wscLine[i+1] == wcSplitChar))
				i++; // skip "whitechar"
		}
	}

	return L"";
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

int ToInt(string scStr)
{
	return atoi(scStr.c_str());
}

namespace HkIEngine
{
    EXPORT bool __stdcall LaunchPos(uint iSpaceID, struct CEqObj &p1, Vector &p2, Matrix &p3, int iDock) 
    {
		returncode = NOFUNCTIONCALL;
		bool iRet = p1.launch_pos(p2,p3,iDock);
	    if(g_bInPlayerLaunch)
	    {
		    p2 = g_Vlaunch;
		    p3 = g_Mlaunch;
	    }
		return true;
    }
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "BS Docking pluging by M0tah";
	p_PI->sShortName = "bsdocking";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process,0));
//	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIEngine::LaunchPos, PLUGIN_LaunchPosHook,0));
    p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch, PLUGIN_HkIServerImpl_PlayerLaunch,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::LaunchComplete, PLUGIN_HkIServerImpl_LaunchComplete,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::BaseEnter, PLUGIN_HkIServerImpl_BaseEnter,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::DisConnect, PLUGIN_HkIServerImpl_DisConnect,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterInfoReq, PLUGIN_HkIServerImpl_CharacterInfoReq,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JumpInComplete, PLUGIN_HkIServerImpl_JumpInComplete,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SystemSwitchOutComplete, PLUGIN_HkIServerImpl_SystemSwitchOutComplete,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::RequestEvent, PLUGIN_HkIServerImpl_RequestEvent,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ShipDestroyed, PLUGIN_ShipDestroyed,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Update, PLUGIN_HkIServerImpl_Update,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_Dock_Call, PLUGIN_HkCb_Dock_Call,0));
	return p_PI;
}