#include "binarytree.h"

#define ADDR_RMCLIENT_LAUNCH 0x5B40
#define ADDR_RMCLIENT_CLIENT 0x43D74

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

struct MOBILE_SHIP
{
	MOBILE_SHIP(uint shipid, int num_occupants) { iShipID = shipid; iMaxNumOccupants = num_occupants; }
	MOBILE_SHIP(uint shipid) { iShipID = shipid; iMaxNumOccupants = 0; }
	uint iShipID;
	int iMaxNumOccupants;
	bool operator==(MOBILE_SHIP ms) { return ms.iShipID==iShipID; }
	bool operator>=(MOBILE_SHIP ms) { return ms.iShipID>=iShipID; }
	bool operator<=(MOBILE_SHIP ms) { return ms.iShipID<=iShipID; }
	bool operator>(MOBILE_SHIP ms) { return ms.iShipID>iShipID; }
	bool operator<(MOBILE_SHIP ms) { return ms.iShipID<iShipID; }
};

struct LAUNCH_PACKET
{
	uint iShip;
	uint iDunno[2];
	float fRotate[4];
	float fPos[3];
};

struct CARGO_REMOVE
{
	uint iGoodID;
	int iCount;
};

struct MDOCK_DATA
{
	bool bMobileDocked;
	uint iDockClientID;
	list<uint>lstJumpPath;
	bool bPathJump;
	mstime tmCharInfoReqAfterDeath;
	Vector Vlaunch;
	Matrix Mlaunch;
	bool bMobileBase;
	int iMaxPlayersDocked;
	list<uint>lstPlayersDocked;
	uint iLastSpaceObjDocked;
	uint iLastEnteredBaseID;
	list<CARGO_REMOVE>lstRemCargo;
	uint iControllerID;
	bool bCheckedDock;
	uint iLastExitedBaseID;
};

struct DOCK_RESTRICTION
{
	DOCK_RESTRICTION(uint obj, uint arch, uint count, wstring denied) { iJumpObjID = obj; iArchID = arch; iCount = count; wscDeniedMsg = denied; }
	DOCK_RESTRICTION(uint obj) { iJumpObjID = obj; }
	uint iJumpObjID;
	uint iArchID;
	int iCount;
	wstring wscDeniedMsg;
	bool operator==(DOCK_RESTRICTION dr) { return dr.iJumpObjID==iJumpObjID; }
	bool operator>=(DOCK_RESTRICTION dr) { return dr.iJumpObjID>=iJumpObjID; }
	bool operator<=(DOCK_RESTRICTION dr) { return dr.iJumpObjID<=iJumpObjID; }
	bool operator>(DOCK_RESTRICTION dr) { return dr.iJumpObjID>iJumpObjID; }
	bool operator<(DOCK_RESTRICTION dr) { return dr.iJumpObjID<iJumpObjID; }
};

struct MOB_UNDOCKBASEKILL
{
	uint iClientID;
	uint iBaseID;
	bool bKill;
};

Quaternion HkMatrixToQuaternion(Matrix m);

HK_ERROR HkInstantDock(wstring wscCharname, uint iDockObj);
HK_ERROR HkBeamInSys(wstring wscCharname, Vector vOffsetVector, Matrix mOrientation);
bool HkIsOkayToDock(uint iClientID, uint iTargetClientID);
float HkDistance3D(Vector v1, Vector v2);
string HkGetPlayerSystemS(uint iClientID);
bool HkDockingRestrictions(uint iClientID, uint iDockObj);
void ClearDOCKData(uint iClientID);

wstring Trim(wstring wscIn);
string Trim(string scIn);
wstring GetParamToEnd(wstring wscLine, wchar_t wcSplitChar, uint iPos);
string GetParamToEnd(string scLine, char cSplitChar, uint iPos);
string GetParam(string scLine, char cSplitChar, uint iPos);
int ToInt(string scStr);

struct PATCH_INFO_ENTRYX
{
	ulong pAddress;
	void *pNewValue;
	uint iSize;
	void *pOldValue;
	bool bAlloced;
};

struct PATCH_INFOX
{
	char	*szBinName;
	ulong	pBaseAddress;

	PATCH_INFO_ENTRYX piEntries[128];
};