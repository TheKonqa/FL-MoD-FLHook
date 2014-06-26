#pragma warning(disable: 4996)
struct AC_DATA
{
	mstime AntiCheatT;
	bool AntiCheat;
	bool IsAdmin;
	bool CrC;
};

void HkTimerAntiCheat();
void HkTimerCrC();

bool HkAddChatLogSpeed(uint iClientID, wstring wscMessage);
bool HkAddChatLogProc(uint iClientID, wstring wscMessage);
bool HkAddChatLogATProc(uint iClientID, wstring wscMessage);
bool HkAddChatLogCRC(uint iClientID, wstring wscMessage);
void CmdFullLog(CCmds* classptr, wstring wscToggle);
void CmdTest(CCmds* classptr);
void CmdATTest(CCmds* classptr);