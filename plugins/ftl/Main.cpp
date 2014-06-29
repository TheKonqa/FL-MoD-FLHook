#include <windows.h>
#include <stdio.h>
#include <string>
#include <FLHook.h>
#include <plugin.h>
#include "header.h"
#include <math.h>

BinaryTree<UINT_WRAP> *set_btBattleShipArchIDs = new BinaryTree<UINT_WRAP>();
BinaryTree<UINT_WRAP> *set_btFreighterShipArchIDs = new BinaryTree<UINT_WRAP>();
BinaryTree<UINT_WRAP> *set_btFighterShipArchIDs = new BinaryTree<UINT_WRAP>();
//FTL
int set_FTLTimer;
list<INISECTIONVALUE> lstFTLFuel;
string sFtlFX;
_GetIObjRW GetIObjRW;
FTL_DATA ftl[250];

PLUGIN_RETURNCODE returncode;

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

//EXPORT void UserCmd_Help(uint iClientID, const wstring &wscParam)
//{
//    PrintUserCmdText(iClientID, L"/ftl <--- you must target a jump hole or gate /ftl x y z");
//	PrintUserCmdText(iClientID, L"/sftl <--- will move you to the location in the system you are in /sftl x y z");
//	PrintUserCmdText(iClientID, L"/j <--- target a object and you will jump to that object");
//	PrintUserCmdText(iClientID, L"/coords <--- supplys the coords of your target to be used with /ftl or /sftl");
//}

EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	list<INISECTIONVALUE> lstValues;
	string set_scFTLFile;
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_scFTLFile = string(szCurDir) + "\\flhook_plugins\\ftl.ini";
	
	IniGetSection(set_scFTLFile, "BattleShip", lstValues);
    set_btBattleShipArchIDs->Clear();
	foreach(lstValues, INISECTIONVALUE, ships)
	{
		uint iShipArchID;
		pub::GetShipID(iShipArchID, ships->scKey.c_str());
		UINT_WRAP *uw = new UINT_WRAP(iShipArchID);
		set_btBattleShipArchIDs->Add(uw);
	}
		
	IniGetSection(set_scFTLFile, "Freighter", lstValues);
    set_btFreighterShipArchIDs->Clear();
	foreach(lstValues, INISECTIONVALUE, ships)
	{
		uint iShipArchID;
		pub::GetShipID(iShipArchID, ships->scKey.c_str());
		UINT_WRAP *uw = new UINT_WRAP(iShipArchID);
		set_btFreighterShipArchIDs->Add(uw);
	}

	IniGetSection(set_scFTLFile, "Fighter", lstValues);
    set_btFighterShipArchIDs->Clear();
	foreach(lstValues, INISECTIONVALUE, ships)
	{
		uint iShipArchID;
		pub::GetShipID(iShipArchID, ships->scKey.c_str());
		UINT_WRAP *uw = new UINT_WRAP(iShipArchID);
		set_btFighterShipArchIDs->Add(uw);
	}

	lstFTLFuel.clear();
	IniGetSection(set_scFTLFile, "FTLFuel", lstFTLFuel);
	set_FTLTimer = IniGetI(set_scFTLFile, "General", "FTLTimer", 0);
	sFtlFX = IniGetS(set_scFTLFile, "General", "ftlfx", "");
	GetIObjRW = (_GetIObjRW)SRV_ADDR(ADDR_SRV_GETIOBJRW);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	LoadSettings();
	return true;
}

//FTL
void UserCmd_FTL(uint iClientID, const wstring &wscParam) 
{
//	GET_USERFILE(scUserFile);
	CAccount *acc = Players.FindAccountFromClientID(iClientID);
	wstring wscDir;
	HkGetAccountDirName(acc, wscDir);
	scUserFile = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
    ftl[iClientID].aFTL = false;
    uint iShip; 
    pub::Player::GetShip(iClientID, iShip);
    uint iTarget; 
    pub::SpaceObj::GetTarget(iShip, iTarget);
    uint iFuseID = CreateID(sFtlFX.c_str());
    IObjRW *ship = GetIObjRW(iShip);
    uint iType;
    pub::SpaceObj::GetType(iTarget, iType);
	uint iShipArchID;
    pub::Player::GetShipID(iClientID, iShipArchID);
    UINT_WRAP uw = UINT_WRAP(iShipArchID);
	bool ShipFound=false;
	if(set_btBattleShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(set_btFreighterShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(set_btFighterShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
    if(ShipFound)
	{
		ftl[iClientID].MsgFTL = true;
        if(iType==64 || iType==2048) //target is jumphole/gate 
		{
            float HullNow, MaxHull, FTLPower;
            pub::SpaceObj::GetHealth(iShip , HullNow, MaxHull);
            FTLPower = (HullNow / MaxHull);	
            if(FTLPower>0.5f)
			{
                if(timeInMS() > ftl[iClientID].iFTL)
				{
                    HkInitFTLFuel(iClientID);
                    if(ftl[iClientID].bHasFTLFuel)
					{
				        IniWrite(scUserFile, "FTL", "coords", wstos(wscParam));
	                    mstime tmFTL = timeInMS();
	                    ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;
                        pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("new_coordinates_received"));
                        HkUnLightFuse(ship, iFuseID, 0.0f);
                        HkLightFuse(ship, iFuseID, 0.0f, 0.5f, -1.0f);
                        ClientInfo[iClientID].tmSpawnTime = timeInMS();
	                    ftl[iClientID].aFTL = true;
						ftl[iClientID].Msg = true;
                        HkInstantDock(ARG_CLIENTID(iClientID), iTarget);
					}
                    else
					{
                        PrintUserCmdText(iClientID, L"Error: you have no Fuel");
					}
				}
                else
				{
                    PrintUserCmdText(iClientID, L"Error: FTL Not Charged Time Left %i ms",(int) (ftl[iClientID].iFTL - timeInMS()));
				}
			}
            else
			{
                PrintUserCmdText(iClientID, L"Ship Repairs needed");
			}
		}
        else 
		{ 
            PrintUserCmdText(iClientID, L"Error: you must use this command on jumpholes/gates"); 
		}
	}
	else
	{
		PrintUserCmdText(iClientID, L"Your ship has no FTL drive");
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FTL
void UserCmd_SFTL(uint iClientID, const wstring &wscParam) 
{
    uint iShip;
    pub::Player::GetShip(iClientID, iShip);
    uint iFuseID = CreateID(sFtlFX.c_str());
    IObjRW *ship = GetIObjRW(iShip);
    float HullNow, MaxHull, FTLPower;
    pub::SpaceObj::GetHealth(iShip , HullNow, MaxHull);
    FTLPower = (HullNow / MaxHull);
	uint iShipArchID;
    pub::Player::GetShipID(iClientID, iShipArchID);
    UINT_WRAP uw = UINT_WRAP(iShipArchID);
	bool ShipFound=false;
	if(set_btBattleShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(set_btFreighterShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(set_btFighterShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(ShipFound)
	{
		ftl[iClientID].MsgFTL = true;
        if(FTLPower>0.5f)
		{
            if(timeInMS() > ftl[iClientID].iFTL)
			{ 
                HkInitFTLFuel(iClientID);
                if(ftl[iClientID].bHasFTLFuel)
				{
                    Vector VLmy; 
                    Matrix MyTemp;
                    pub::SpaceObj::GetLocation(iShip, VLmy, MyTemp); 
                    VLmy.x = 0;
                    VLmy.y = 0;
                    VLmy.z = 0;
                    wstring Vx = GetParam(wscParam, ' ', 0);
                    if(Vx.length() < 8)
					{VLmy.x = ToFloat(Vx);} 
                    wstring Vy = GetParam(wscParam, ' ', 1); 
                    if(Vy.length() < 8)
					{VLmy.y = ToFloat(Vy);}
                    wstring Vz = GetParam(wscParam, ' ', 2); 
                    if(Vz.length() < 8)
					{VLmy.z = ToFloat(Vz);}
                    ClientInfo[iClientID].tmSpawnTime = timeInMS();
                    pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("new_coordinates_received")); 
                    HkUnLightFuse(ship, iFuseID, 0.0f);
                    HkLightFuse(ship, iFuseID, 0.0f, 0.5f, -1.0f);
					pub::SpaceObj::SetInvincible(iShip, true, true, 0);
                    HkBeamInSys(ARG_CLIENTID(iClientID), VLmy, MyTemp);
					pub::SpaceObj::SetInvincible(iShip, false, false, 0);
	                HkUnLightFuse(ship, iFuseID, 0.0f);
                    HkLightFuse(ship, iFuseID, 0.0f, 0.5f, -1.0f);
	                pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("launch_procedure_complete"));
	                mstime tmFTL = timeInMS();
	                ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;
                    ftl[iClientID].bHasFTLFuel = false;
				    ftl[iClientID].Msg = true;
				}
                else
				{
                    PrintUserCmdText(iClientID, L"Error: you have no Fuel");
				}
			}
            else
			{
                PrintUserCmdText(iClientID, L"Error: FTL Not Charged Time Left %i ms",(int) (ftl[iClientID].iFTL - timeInMS()));
			}
		}
        else
		{
            PrintUserCmdText(iClientID, L"Ship Repairs needed");
		}
	}
	else
	{
		PrintUserCmdText(iClientID, L"Your ship has no FTL drive");
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FTL
void UserCmd_COORDS(uint iClientID, const wstring &wscParam) 
{
    uint iShip; 
    pub::Player::GetShip(iClientID, iShip);  
    uint iTarget; 
    pub::SpaceObj::GetTarget(iShip, iTarget);  
    Vector myLocation;
    Matrix myLocationm;
    pub::SpaceObj::GetLocation(iTarget, myLocation, myLocationm);
    PrintUserCmdText(iClientID, L"x %f",myLocation.x);
    PrintUserCmdText(iClientID, L"y %f",myLocation.y);
    PrintUserCmdText(iClientID, L"z %f",myLocation.z);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//FTL
void UserCmd_JUMP(uint iClientID, const wstring &wscParam) 
{
    uint iShip;
    pub::Player::GetShip(iClientID, iShip);
    uint iFuseID = CreateID(sFtlFX.c_str());
	IObjRW *ship = GetIObjRW(iShip);
    float HullNow, MaxHull, FTLPower;
    pub::SpaceObj::GetHealth(iShip , HullNow, MaxHull);
    FTLPower = (HullNow / MaxHull);
	uint iShipArchID;
    pub::Player::GetShipID(iClientID, iShipArchID);
    UINT_WRAP uw = UINT_WRAP(iShipArchID);
	bool ShipFound=false;
	if(set_btBattleShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(set_btFreighterShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(set_btFighterShipArchIDs->Find(&uw))
	{
		ShipFound=true;
	}
	if(ShipFound)
	{
		ftl[iClientID].MsgFTL = true;
	    if(FTLPower>0.5f)
		{
            if(timeInMS() > ftl[iClientID].iFTL)
			{ 
                HkInitFTLFuel(iClientID);
                if(ftl[iClientID].bHasFTLFuel)
				{
                    Vector myJump; 
                    Matrix myJumpx;
                    pub::SpaceObj::GetLocation(iShip, myJump, myJumpx); 
                    myJump.x = 0;
                    myJump.y = 0;
                    myJump.z = 0;
                    uint iTarget;
                    pub::SpaceObj::GetTarget(iShip, iTarget);  
                    pub::SpaceObj::GetLocation(iTarget, myJump, myJumpx);
                    ClientInfo[iClientID].tmSpawnTime = timeInMS();
                    pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("new_coordinates_received"));
                    HkUnLightFuse(ship, iFuseID, 0.0f);
                    HkLightFuse(ship, iFuseID, 0.0f, 0.5f, -1.0f);
					pub::SpaceObj::SetInvincible(iShip, true, true, 0);
                    HkBeamInSys(ARG_CLIENTID(iClientID), myJump, myJumpx);
					pub::SpaceObj::SetInvincible(iShip, false, false, 0);
	                HkUnLightFuse(ship, iFuseID, 0.0f);
                    HkLightFuse(ship, iFuseID, 0.0f, 0.5f, -1.0f);
	                pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("launch_procedure_complete"));
	                mstime tmFTL = timeInMS();
	                ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;
                    ftl[iClientID].bHasFTLFuel = false;
	                ftl[iClientID].Msg = true;
				}
                else
				{
                    PrintUserCmdText(iClientID, L"Error: you have no Fuel");
				}
			}
            else
			{
                PrintUserCmdText(iClientID, L"Error: FTL Not Charged Time Left %i ms",(int) (ftl[iClientID].iFTL - timeInMS()));
			}
   
		}
        else
		{
            PrintUserCmdText(iClientID, L"Ship Repairs needed");
		}
	}
	else
	{
		PrintUserCmdText(iClientID, L"Your ship has no FTL drive");
	}
}

HK_ERROR HkInitFTLFuel(uint iClientID)
{
	ftl[iClientID].bHasFTLFuel = false;
	int add=1;
    uint iShipArchID;
    pub::Player::GetShipID(iClientID, iShipArchID);
    UINT_WRAP uw = UINT_WRAP(iShipArchID);
	if(set_btBattleShipArchIDs->Find(&uw))
	{
		add=3;
	}
	if(set_btFreighterShipArchIDs->Find(&uw))
	{
		add=2;
	}
	list <CARGO_INFO> lstCargo;
	int iRem;
	HkEnumCargo(ARG_CLIENTID(iClientID), lstCargo, iRem);
	foreach(lstCargo, CARGO_INFO, it)
	{
	    foreach(lstFTLFuel,INISECTIONVALUE,it2)
		{
			uint iFTLFuel = CreateID(it2->scKey.c_str());
			int FuelAmount = ToInt(stows(it2->scValue));
			FuelAmount*=add;
			if (it->iArchID == iFTLFuel && it->iCount >= FuelAmount)
			{
	            HkRemoveCargo(ARG_CLIENTID(iClientID), it->iID, FuelAmount);
	            ftl[iClientID].bHasFTLFuel = true;
				return HKE_OK;
			}
		}

	}
	return HKE_OK;
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

HK_ERROR HkInstantDock(wstring wscCharname, uint iDockObj)
{
	HK_GET_CLIENTID(iClientID, wscCharname);

	// check if logged in
	if(iClientID == -1)
		return HKE_PLAYER_NOT_LOGGED_IN;

	uint iShip;
	pub::Player::GetShip(iClientID, iShip);
	if(!iShip)
		return HKE_PLAYER_NOT_IN_SPACE;

	uint iSystemID, iSystemID2;
	pub::SpaceObj::GetSystem(iShip, iSystemID);
	pub::SpaceObj::GetSystem(iDockObj, iSystemID2);
	try {
		pub::SpaceObj::InstantDock(iShip, iDockObj, 1);
	} catch(...) {}
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

namespace HkIServerImpl
{
    EXPORT void __stdcall JumpInComplete_AFTER(unsigned int iSystemID, unsigned int iShip)
    {
		returncode = DEFAULT_RETURNCODE;
	    uint iClientID = HkGetClientIDByShip(iShip);
	    if(ftl[iClientID].aFTL) 
		{
			CAccount *acc = Players.FindAccountFromClientID(iClientID);
	        wstring wscDir;
	        HkGetAccountDirName(acc, wscDir);
	        scUserFile = scAcctPath + wstos(wscDir) + "\\flhookuser.ini";
            uint iShip; 
            pub::Player::GetShip(iClientID, iShip);
	        uint iFuseID = CreateID(sFtlFX.c_str());
	        IObjRW *ship = GetIObjRW(iShip);
            Matrix MTemp;
            Vector VTemp;
            pub::SpaceObj::GetLocation(iShip, VTemp,MTemp);
            ClientInfo[iClientID].tmSpawnTime = timeInMS();
			wstring coords = stows(IniGetS(scUserFile, "FTL", "coords", ""));
            wstring Vx = GetParam(coords, ' ', 0);
            if(Vx.length() < 8)
			{VTemp.x = ToFloat(Vx);} 
            wstring Vy = GetParam(coords, ' ', 1);
            if(Vy.length() < 8) 
			{VTemp.y = ToFloat(Vy);} 
             wstring Vz = GetParam(coords, ' ', 2);
            if(Vz.length() < 8) 
			{VTemp.z = ToFloat(Vz);}
            HkBeamInSys(ARG_CLIENTID(iClientID), VTemp, MTemp);
            ftl[iClientID].bHasFTLFuel = false;
            HkUnLightFuse(ship, iFuseID, 0.0f);
            HkLightFuse(ship, iFuseID, 0.0f, 0.5f, -1.0f);
	        pub::Player::SendNNMessage(iClientID, pub::GetNicknameId("launch_procedure_complete"));
	        ftl[iClientID].aFTL = false;
			IniDelSection(scUserFile, "FTL");
		}
	}

	typedef void (*_TimerFunc)();
	struct TIMER
	{
		_TimerFunc	proc;
		mstime		tmIntervallMS;
		mstime		tmLastCall;
	};

	TIMER Timers[] = 
	{
		{FTLMsgPlayers,			50,				0},
	};

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
				ClearFTLData(HkGetClientIdFromPD(pPD));
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
	EXPORT void __stdcall PlayerLaunch(unsigned int iShip, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;
		ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;
        ftl[iClientID].Msg = true;
	}
	EXPORT void __stdcall CharacterSelect_AFTER(struct CHARACTER_ID const & cId, unsigned int iClientID)
	{
		returncode = DEFAULT_RETURNCODE;
		ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;
        ftl[iClientID].Msg = true;
		ftl[iClientID].MsgFTL = false;
	}
}

EXPORT int __stdcall HkCB_MissileTorpHit(char *ECX, char *p1, DamageList *dmg)
{
	char *szP;
	memcpy(&szP, ECX + 0x10, 4);
	uint iClientID;
	memcpy(&iClientID, szP + 0xB4, 4);
    iDmgTo = iClientID;
	if(iClientID)
	{
	    if(((dmg->get_cause() == 6) || (dmg->get_cause() == 0x15)))
	    {
		    ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;
		    if(ftl[iClientID].MsgFTL)
		    {
			    PrintUserCmdText(iClientID, L"FTL Drive disrupted");
				ftl[iClientID].Msg = true;
		    }
	    }
	}
	return 0;
}

void ClearFTLData(uint iClientID)
{
	ftl[iClientID].iFTL = timeInMS() + set_FTLTimer;;
	ftl[iClientID].aFTL = false;
	ftl[iClientID].bHasFTLFuel = false;
    ftl[iClientID].Msg = false;
	ftl[iClientID].MsgFTL = false;
}

void FTLMsgPlayers()
{
	struct PlayerData *pPD = 0;
	while(pPD = Players.traverse_active(pPD))
	{
		uint iClientID = HkGetClientIdFromPD(pPD);
		if(ClientInfo[iClientID].tmF1TimeDisconnect)
			continue;

		DPN_CONNECTION_INFO ci;
		if(HkGetConnectionStats(iClientID, ci) != HKE_OK)
			continue;

		if(timeInMS() > ftl[iClientID].iFTL && ftl[iClientID].MsgFTL)
		{
			if(ftl[iClientID].Msg)
			{
                PrintUserCmdText(iClientID, L"FTL Ready to be Activated");
				ftl[iClientID].Msg = false;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

__declspec(naked) void __stdcall HkLightFuse(IObjRW *ship, uint iFuseID, float fDelay, float fLifetime, float fSkip)
{
	__asm
	{
		lea eax, [esp+8] //iFuseID
		push [esp+20] //fSkip
		push [esp+16] //fDelay
		push 0 //SUBOBJ_ID_NONE
		push eax
		push [esp+32] //fLifetime
		mov ecx, [esp+24]
		mov eax, [ecx]
		call [eax+0x1E4]
		ret 20
	}
}

__declspec(naked) void __stdcall HkUnLightFuse(IObjRW *ship, uint iFuseID, float fDunno)
{
	__asm
	{
		mov ecx, [esp+4]
		lea eax, [esp+8] //iFuseID
		push [esp+12] //fDunno
		push 0 //SUBOBJ_ID_NONE
		push eax //iFuseID
		mov eax, [ecx]
		call [eax+0x1E8]
		ret 12
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*_UserCmdProc)(uint, const wstring &);

struct USERCMD
{
	wchar_t *wszCmd;
	_UserCmdProc proc;
};

USERCMD UserCmds[] =
{
	{ L"/ftl",				UserCmd_FTL},
	{ L"/sftl",			    UserCmd_SFTL},
	{ L"/j",			    UserCmd_JUMP},
	{ L"/coords",           UserCmd_COORDS},
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
	p_PI->sName = "ftl jump plugin by kosacid";
	p_PI->sShortName = "ftl";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = false;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Process, PLUGIN_UserCmd_Process,0));
//	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&UserCmd_Help, PLUGIN_UserCmd_Help,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Update, PLUGIN_HkIServerImpl_Update,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::PlayerLaunch, PLUGIN_HkIServerImpl_PlayerLaunch,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::JumpInComplete_AFTER, PLUGIN_HkIServerImpl_JumpInComplete_AFTER,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::CharacterSelect_AFTER, PLUGIN_HkIServerImpl_CharacterSelect_AFTER,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCB_MissileTorpHit, PLUGIN_HkCB_MissileTorpHit,0));
	return p_PI;
}