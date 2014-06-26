#include "binarytree.h"

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

struct CARGO_REMOVE
{
	uint iGoodID;
	int iCount;
};

struct DOCK_DATA
{
	bool bCheckedDock;
	list<CARGO_REMOVE> lstRemCargo;
};
bool HkDockingRestrictions(uint iClientID, uint iDockObj);
string GetParamToEnd(string scLine, char cSplitChar, uint iPos);
string Trim(string scIn);
string GetParam(string scLine, char cSplitChar, uint iPos);