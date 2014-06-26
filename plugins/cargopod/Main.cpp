#include <windows.h>
#include <stdio.h>
#include <string>
#include "../flhookplugin_sdk/headers/FLHook.h"
#include "../flhookplugin_sdk/headers/plugin.h"
#include "header.h"
#include <vector>
#define ADDR_FLCONFIG 0x25410

vector<HINSTANCE> vDLLs;
BinaryTree<CARGO_POD> *set_btCargoPod = new BinaryTree<CARGO_POD>();
PLUGIN_RETURNCODE returncode;
uint iClientIDTarget;
uint iClientIDKiller;
TRAN_DATA trans[250];
bool PlayerHit=false;

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;
	list<INISECTIONVALUE> value;
	string set_scCargoPod;
	char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scCargoPod = string(szCurDir) + "\\flhook_plugins\\CargoPod.ini";
	IniGetSection(set_scCargoPod, "CargoPod", value);
	set_btCargoPod->Clear();
	foreach(value, INISECTIONVALUE, lst)
	{
		uint PodID;
		PodID = CreateID(lst->scKey.c_str());
		CARGO_POD *pod = new CARGO_POD(PodID,atoi(lst->scValue.c_str()));
		set_btCargoPod->Add(pod);
	}
	char *szFLConfig = (char*)((char*)GetModuleHandle(0) + ADDR_FLCONFIG);
	HkLoadDLLConf(szFLConfig);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

	if(fdwReason == DLL_PROCESS_ATTACH)
		LoadSettings();

	return true;
}

wstring HkGetWStringFromIDS(uint iIDS) //Only works for names
{
	if(!iIDS)
		return L"";

	uint iDLL = iIDS / 0x10000;
	iIDS -= iDLL * 0x10000;

	wchar_t wszBuf[512];
	if(LoadStringW(vDLLs[iDLL], iIDS, wszBuf, 512))
		return wszBuf;
	return L"";
}

void HkLoadDLLConf(const char *szFLConfigFile)
{
	for(uint i=0; i<vDLLs.size(); i++)
	{
		FreeLibrary(vDLLs[i]);
	}
	vDLLs.clear();
	HINSTANCE hDLL = LoadLibraryEx((char*)((char*)GetModuleHandle(0) + 0x256C4), NULL, LOAD_LIBRARY_AS_DATAFILE); //typically resources.dll
	if(hDLL)
		vDLLs.push_back(hDLL);
	INI_Reader ini;
	if(ini.open(szFLConfigFile, false))
	{
		while(ini.read_header())
		{
			if(ini.is_header("Resources"))
			{
				while(ini.read_value())
				{
					if(ini.is_value("DLL"))
					{
						hDLL = LoadLibraryEx(ini.get_value_string(0), NULL, LOAD_LIBRARY_AS_DATAFILE);
						if(hDLL)
							vDLLs.push_back(hDLL);
					}
				}
			}
		}
		ini.close();
	}
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

string utos(uint i)
{
	char szBuf[16];
	sprintf_s(szBuf, "%u", i);
	return szBuf;
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
		{GoodsTranfer,			100,				0},
	};

	EXPORT int __stdcall Update()
	{
		returncode = DEFAULT_RETURNCODE;
		static bool bFirstTime = true;
		if(bFirstTime) 
		{
			bFirstTime = false;
			struct PlayerData *pPD = 0; 
			while(pPD = Players.traverse_active(pPD))
				ClearTranData(HkGetClientIdFromPD(pPD));
		}
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

	EXPORT void __stdcall SPMunitionCollision(struct SSPMunitionCollisionInfo const & ci, unsigned int iClientID)
    {
		returncode = DEFAULT_RETURNCODE;
		iClientIDTarget = HkGetClientIDByShip(ci.dwTargetShip);
		iClientIDKiller = iClientID;
		if(iClientIDTarget)
		{
			PlayerHit=true;
		}
		else
		{
			PlayerHit=false;
		}
	}

	EXPORT void __stdcall ReqRemoveItem(unsigned short p1, int p2, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;
		CAccount *acc = Players.FindAccountFromClientID(iClientID);
	    wstring wscDir;
	    HkGetAccountDirName(acc, wscDir);
	    scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
		wstring wscFilename;
		HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
		string scSection = utos(p1) + "_" + wstos(wscFilename);
		IniDelSection(scUserStore,scSection);
	}

	EXPORT void __stdcall GFGoodSell_AFTER(struct SGFGoodSellInfo const &gsi, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;
		trans[iClientID].bHold = true;
	}

	EXPORT void __stdcall GFGoodBuy_AFTER(struct SGFGoodBuyInfo const &gbi, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;
		list<CARGO_INFO> lstCargo;
		int iRem;
		int counter=0;
	    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
		CAccount *acc = Players.FindAccountFromClientID(iClientID);
	    wstring wscDir;
	    HkGetAccountDirName(acc, wscDir);
	    scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
		wstring wscFilename;
		HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
		IniDelSection(scUserStore,"pods_" + wstos(wscFilename));
		foreach(lstCargo, CARGO_INFO, cargo)
	    {
		    const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		    if(!gi)
		       continue;
		    if(cargo->bMounted && gi->iIDS)
		    {
			    CARGO_POD FindPod = CARGO_POD(cargo->iArchID, 0);
	            CARGO_POD *pod = set_btCargoPod->Find(&FindPod);
				if(cargo->iID && pod)
				{
					trans[iClientID].isPod=true;
		            string scSection = utos(cargo->iID) + "_" + wstos(wscFilename);
					IniWrite(scUserStore, "pods_" + wstos(wscFilename),itos(cargo->iID),"1");
					int Capacity=0;
					list<INISECTIONVALUE> goods;
					goods.clear();
					IniGetSection(scUserStore, scSection, goods);
					foreach(goods,INISECTIONVALUE,lst)
					{
						Capacity+=ToInt(stows(lst->scValue).c_str());
					}
					if(Capacity<pod->capacity)
					{
						list<CARGO_INFO> lstCargoNew;
						int iRem;
	                    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargoNew, iRem);
					    foreach(lstCargoNew, CARGO_INFO, cargo)
					    {   
						    const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		                    if(!gi)
		                    continue;
						    Archetype::Equipment *eq = Archetype::GetEquipment(cargo->iArchID);
						    if(!cargo->bMounted && gi->iIDS && gi->iType == 0)
						    {
							    if(cargo->iCount > pod->capacity-Capacity)
							    {
								    int iGoods = IniGetI(scUserStore, scSection,utos(cargo->iArchID),0);
                                    IniWrite(scUserStore, scSection, utos(cargo->iArchID), itos(pod->capacity-Capacity+iGoods));
				                    HkRemoveCargo(ARG_CLIENTID(iClientID), cargo->iID, pod->capacity-Capacity);
							    }
							    else
							    {
								    int iGoods = IniGetI(scUserStore, scSection,utos(cargo->iArchID),0);
								    IniWrite(scUserStore, scSection, utos(cargo->iArchID), itos(cargo->iCount+iGoods));
				                    HkRemoveCargo(ARG_CLIENTID(iClientID), cargo->iID, cargo->iCount);
							    }
						    }
					    }
					}
			    }
		    }      
	    }
	}

	EXPORT void __stdcall CharacterSelect_AFTER(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		CAccount *acc = Players.FindAccountFromClientID(iClientID);
	    wstring wscDir;
	    HkGetAccountDirName(acc, wscDir);
	    scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
		wstring wscFilename;
		HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
		IniDelSection(scUserStore,"pods_" + wstos(wscFilename));
		list<CARGO_INFO> lstCargo;
		int iRem;
	    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
		foreach(lstCargo, CARGO_INFO, cargo)
	    {
		    const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		    if(!gi)
		       continue;
		    if(cargo->bMounted && gi->iIDS)
		    {
			    CARGO_POD FindPod = CARGO_POD(cargo->iArchID, 0);
	            CARGO_POD *pod = set_btCargoPod->Find(&FindPod);
				if(cargo->iID && pod)
				{
					trans[iClientID].isPod=true;
					IniWrite(scUserStore, "pods_" + wstos(wscFilename),itos(cargo->iID),"1");
				}
			}
		}

	}
}

EXPORT void __stdcall HkCb_AddDmgEntry(DamageList *dmgList, unsigned short p1, float p2, enum DamageEntry::SubObjFate p3)
{
	returncode = DEFAULT_RETURNCODE;
	
	if(PlayerHit)
	{
		CAccount *acc = Players.FindAccountFromClientID(iClientIDTarget);
	    wstring wscDir;
	    HkGetAccountDirName(acc, wscDir);
	    scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
		wstring wscFilename;
		HkGetCharFileName(ARG_CLIENTID(iClientIDTarget), wscFilename);
		int iPod = IniGetI(scUserStore, "pods_" + wstos(wscFilename),itos(p1),0);
		if(p1>1 && p1<65521 && dmgList->is_inflictor_a_player() && iPod==1 && p2==0.000000f)
		{
		    string scSection = utos(p1) + "_" + wstos(wscFilename);
			PrintUserCmdText(iClientIDTarget,L"Pod Destroyed slot number %u all goods destroyed",p1);
			trans[iClientIDTarget].bHold=true;
			list<INISECTIONVALUE> goods;
			IniGetSection(scUserStore, scSection, goods);
			uint iSystem = 0;
			pub::Player::GetSystem(iClientIDTarget, iSystem);
			uint iShip = 0;
			pub::Player::GetShip(iClientIDTarget, iShip);  
			Vector vLoc = { 0.0f, 0.0f, 0.0f };
			Matrix mRot = { 0.0f, 0.0f, 0.0f };
			pub::SpaceObj::GetLocation(iShip, vLoc, mRot);
			vLoc.x += 30.0;
			static int set_iLootCrateID=CreateID("lootcrate_ast_loot_metal");
			foreach(goods,INISECTIONVALUE,lst)
			{
				int ammount = ToInt(stows(lst->scValue).c_str());
				Archetype::Equipment *eq = Archetype::GetEquipment(ToInt(stows(lst->scKey).c_str()));
				Server.MineAsteroid(iSystem, vLoc, set_iLootCrateID, eq->iArchID, ammount, iClientIDTarget);
			}
			IniDelSection(scUserStore,scSection);
			PlayerHit=false;
		}
	}
    else
	{
		PlayerHit=false;
	}
}

EXPORT void __stdcall ShipDestroyed(DamageList *_dmg, char *szECX, uint iKill)
{
	returncode = DEFAULT_RETURNCODE;

	char *szP;
	memcpy(&szP, szECX + 0x10, 4);
	uint iClientID;
	memcpy(&iClientID, szP + 0xB4, 4);
	if(trans[iClientID].isPod)
	{
		list<CARGO_INFO> lstCargo;
		int iRem;
	    HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
		foreach(lstCargo, CARGO_INFO, cargo)
	    {
		    const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		    if(!gi)
		       continue;
		    if(cargo->bMounted && gi->iIDS)
		    {
			    CARGO_POD FindPod = CARGO_POD(cargo->iArchID, 0);
	            CARGO_POD *pod = set_btCargoPod->Find(&FindPod);
				if(cargo->iID && pod)
				{
					CAccount *acc = Players.FindAccountFromClientID(iClientID);
	                wstring wscDir;
	                HkGetAccountDirName(acc, wscDir);
	                scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
		            wstring wscFilename;
		            HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
		            string scSection = utos(cargo->iID) + "_" + wstos(wscFilename);
					IniDelSection(scUserStore,scSection);
				}
			}
		}
	}

}

void ClearTranData(uint iClientID)
{
	trans[iClientID].bHold=false;
	trans[iClientID].isPod=false;
}

void GoodsTranfer()
{
	struct PlayerData *pPD = 0;
	while(pPD = Players.traverse_active(pPD))
	{
		uint iClientID = HkGetClientIdFromPD(pPD);
		if(trans[iClientID].bHold)
		{
	        int iRem;
	        list<CARGO_INFO> lstCargo;
	        lstCargo.clear();
	        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
		    foreach(lstCargo, CARGO_INFO, cargo)
	        {
		        const GoodInfo *gi = GoodList::find_by_id(cargo->iArchID);
		        if(!gi)
		           continue;
		        if(cargo->bMounted && gi->iIDS)
		        {
			        CARGO_POD FindPod = CARGO_POD(cargo->iArchID, 0);
	                CARGO_POD *pod = set_btCargoPod->Find(&FindPod);
				    if(cargo->iID && pod && trans[iClientID].bHold)
				    {
					    CAccount *acc = Players.FindAccountFromClientID(iClientID);
	                    wstring wscDir;
	                    HkGetAccountDirName(acc, wscDir);
	                    scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
		                wstring wscFilename;
		                HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
		                string scSection = utos(cargo->iID) + "_" + wstos(wscFilename);
					    list<INISECTIONVALUE> goods;
					    goods.clear();
					    IniGetSection(scUserStore, scSection, goods);
					    IniDelSection(scUserStore,scSection);
					    uint iMission=0;
					    foreach(goods,INISECTIONVALUE,lst)
					    {
						    int iGoods=0;
						    int iRem;
	                        list<CARGO_INFO> lstCargo;
	                        HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
						    Archetype::Equipment *eq = Archetype::GetEquipment(ToInt(stows(lst->scKey).c_str()));
						    int GoodCounter = ToInt(stows(lst->scValue).c_str());
						    if(iRem<eq->fVolume*GoodCounter)
						    {
							    iGoods=iRem/(int)eq->fVolume;
						    }
						    else
						    {
							    iGoods=GoodCounter;
						    }
						    if(iGoods>0 && trans[iClientID].bHold)
						    {
								pub::Player::AddCargo(iClientID, ToInt(stows(lst->scKey.c_str())), iGoods, 1, false);
								IniWrite(scUserStore, scSection, lst->scKey, itos(GoodCounter-iGoods));
								trans[iClientID].bHold=false;
						    }
							else if(GoodCounter>0)
							{
								IniWrite(scUserStore, scSection, lst->scKey, lst->scValue);
							}
					    }
				    }
			    }
		    }
		    trans[iClientID].bHold=false;
	    }
	}
}

void UserCmd_Pods(uint iClientID, const wstring &wscParam)
{
	list<CARGO_INFO> lstCargo;
	int iRem;
	HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
	uint iNum = 0;
	PrintUserCmdText(iClientID,L"[Hold]");
	foreach(lstCargo, CARGO_INFO, it)
	{
		const GoodInfo *gi = GoodList::find_by_id(it->iArchID);
		if(!gi)
		continue;
		if(!it->bMounted && gi->iIDS)
		{
			PrintUserCmdText(iClientID, L"  %s=%u", HkGetWStringFromIDS(gi->iIDSName).c_str(), it->iCount);
			iNum++;
		}
	}
	if(!iNum)
	PrintUserCmdText(iClientID, L"  you have no goods in your hold");
	foreach(lstCargo, CARGO_INFO, it)
	{
		const GoodInfo *gi = GoodList::find_by_id(it->iArchID);
		if(!gi)
		   continue;
		if(it->bMounted && gi->iIDS)
		{
			CARGO_POD FindPod = CARGO_POD(it->iArchID, 0);
	        CARGO_POD *pod = set_btCargoPod->Find(&FindPod);
			if(it->iID && pod)
			{
				CAccount *acc = Players.FindAccountFromClientID(iClientID);
	            wstring wscDir;
	            HkGetAccountDirName(acc, wscDir);
	            scUserStore = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
	            wstring wscFilename;
	            HkGetCharFileName(ARG_CLIENTID(iClientID), wscFilename);
	            string scSection = utos(it->iID) + "_" + wstos(wscFilename);
				int counter=0;
				list<INISECTIONVALUE> amount;
				amount.clear();
				IniGetSection(scUserStore, scSection, amount);
				foreach(amount,INISECTIONVALUE,lstI)
				{
					counter+=ToInt(stows(lstI->scValue).c_str());
				}
				PrintUserCmdText(iClientID,L"[Cargo Pod = %s capacity left = %i health = %f]",HkGetWStringFromIDS(gi->iIDSName).c_str(),pod->capacity-counter,it->fStatus);
				IniDelSection(scUserStore,scSection);
				foreach(amount,INISECTIONVALUE,lstgoods)
				{
					if(ToInt(stows(lstgoods->scValue).c_str())>0)
					{
						IniWrite(scUserStore, scSection, lstgoods->scKey, lstgoods->scValue);
					    const GoodInfo *ls = GoodList::find_by_id(ToInt(stows(lstgoods->scKey).c_str()));;
		                if(!ls)
		                   continue;
					    PrintUserCmdText(iClientID, L"  %s=%s", HkGetWStringFromIDS(ls->iIDSName).c_str(), stows(lstgoods->scValue).c_str());
					}
				}
			}
		}      
	}
}

EXPORT void UserCmd_Help(uint iClientID, const wstring &wscParam)
{
	PrintUserCmdText(iClientID, L"/pods <--shows a list of cargopods mounted + there stats");
}

typedef void (*_UserCmdProc)(uint, const wstring &);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
};

USERCMD UserCmds[] =
{
	{ L"/pods",				UserCmd_Pods},
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

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "cargopod plugin by kosacid";
	p_PI->sShortName = "cargopod";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::SPMunitionCollision, PLUGIN_HkIServerImpl_SPMunitionCollision,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_AddDmgEntry, PLUGIN_HkCb_AddDmgEntry,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodBuy_AFTER, PLUGIN_HkIServerImpl_GFGoodBuy_AFTER,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::GFGoodSell_AFTER, PLUGIN_HkIServerImpl_GFGoodSell_AFTER,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Update, PLUGIN_HkIServerImpl_Update,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::ReqRemoveItem, PLUGIN_HkIServerImpl_ReqRemoveItem,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER,0));
//	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ShipDestroyed, PLUGIN_ShipDestroyed,0));
	
	return p_PI;
}