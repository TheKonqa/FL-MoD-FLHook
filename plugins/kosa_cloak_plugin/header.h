#include <vector>
wstring HkGetWStringFromIDS(uint iIDS);
void HkLoadDLLConf(const char *szFLConfigFile);
char HkUnMarkObject(uint iClientID, uint iObject);
struct CLOAK_INFO
{
	uint		iCloakingTime;
	uint		iCloakWarmup;
	uint		iCloakCooldown;
	uint		iCloakSlot;
	bool		bCanCloak;
	bool		bCloaked;
	bool		bWantsCloak;
	bool		bIsCloaking;
	mstime		tmCloakTime;
	bool		bMustSendUncloak;
	vector<uint> vMarkedObjs;
    vector<uint> vAutoMarkedObjs;
	vector<uint> vDelayedSystemMarkedObjs;
};
void ClearCloakInfo(uint iClientID);


