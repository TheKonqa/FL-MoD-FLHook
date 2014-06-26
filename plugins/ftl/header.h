#include "binarytree.h"
#define ADDR_SRV_GETIOBJRW 0x20670 // 06D00670
#define SRV_ADDR(a) ((char*)hModServer + a)
typedef IObjRW * (__cdecl *_GetIObjRW)(uint iShip);
extern _GetIObjRW GetIObjRW;

#define ADDR_RMCLIENT_LAUNCH 0x5B40
#define ADDR_RMCLIENT_CLIENT 0x43D74

string scUserFile;

struct UINT_WRAP
{
	UINT_WRAP(uint u) {val = u;}
	bool operator==(UINT_WRAP uw) { return uw.val==val; }
	bool operator>=(UINT_WRAP uw) { return uw.val>=val; }
	bool operator<=(UINT_WRAP uw) { return uw.val<=val; }
	bool operator>(UINT_WRAP uw) { return uw.val>val; }
	bool operator<(UINT_WRAP uw) { return uw.val<val; }
	uint val;
};

struct LAUNCH_PACKET
{
	uint iShip;
	uint iDunno[2];
	float fRotate[4];
	float fPos[3];
};

struct FTL_DATA
{
	bool aFTL;
	mstime iFTL;
	bool bHasFTLFuel;
	bool Msg;
	bool MsgFTL;
};

Quaternion HkMatrixToQuaternion(Matrix m);

HK_ERROR HkInitFTLFuel(uint iClientID);
HK_ERROR HkInstantDock(wstring wscCharname, uint iDockObj);
HK_ERROR HkBeamInSys(wstring wscCharname, Vector vOffsetVector, Matrix mOrientation);
void FTLMsgPlayers();
void ClearFTLData(uint iClientID);

void __stdcall HkLightFuse(IObjRW *ship, uint iFuseID, float fDelay, float fLifetime, float fSkip);
void __stdcall HkUnLightFuse(IObjRW *ship, uint iFuseID, float fDelay);