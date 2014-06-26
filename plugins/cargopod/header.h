#include <set>

wstring HkGetWStringFromIDS(uint iIDS);
void HkLoadDLLConf(const char *szFLConfigFile);

#include "binarytree.h"

struct CARGO_POD
{
	CARGO_POD(uint p, int c) { capacity = c; CargoPodID = p; }
	int	capacity;
	uint	CargoPodID;
	bool operator==(CARGO_POD pd) { return pd.CargoPodID==CargoPodID; }
	bool operator>=(CARGO_POD pd) { return pd.CargoPodID>=CargoPodID; }
	bool operator<=(CARGO_POD pd) { return pd.CargoPodID<=CargoPodID; }
	bool operator>(CARGO_POD pd) { return pd.CargoPodID>CargoPodID; }
	bool operator<(CARGO_POD pd) { return pd.CargoPodID<CargoPodID; }
};

string scUserStore;
string GetParam(string scLine, char cSplitChar, uint iPos);
string utos(uint i);
void GoodsTranfer();
struct TRAN_DATA
{
	bool isPod;
	bool bHold;
};
void ClearTranData(uint iClientID);