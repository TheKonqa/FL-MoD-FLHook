#include <windows.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <FLHook.h>
#include <plugin.h>
#include "header.h"
#include <vector>
#define ADDR_FLCONFIG 0x25410
vector<HINSTANCE> vDLLs;
PLUGIN_RETURNCODE returncode;
list<INISECTIONVALUE> lstFLPaths;
EXPORT void LoadSettings()
{
	returncode = DEFAULT_RETURNCODE;

	string set_Freelancer;
    char szCurDir[MAX_PATH];
    GetCurrentDirectory(sizeof(szCurDir), szCurDir);
    set_Freelancer = string(szCurDir) + "\\freelancer.ini";
	IniGetSection(set_Freelancer, "Data", lstFLPaths);
	char *szFLConfig = (char*)((char*)GetModuleHandle(0) + ADDR_FLCONFIG);
	HkLoadDLLConf(szFLConfig);
	DoHashList();
}

EXPORT PLUGIN_RETURNCODE Get_PluginReturnCode()
{
	return returncode;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	LoadSettings();
	return true;
}

string HashName(uint HashID)
{
	string line;
	string Hash;
	string scHashID = utos(HashID);
	ifstream readhash("FLHash.ini");
	if(readhash.is_open())
	{
		while(readhash.good())
		{
			getline (readhash,line);
			if(line.find(scHashID)==0)
			{
				Hash = Trim(GetParam(line, '=', 1));
			}
		}
		readhash.close();
	}
	return Hash;
}

void DoHashList()
{
	typedef DWORD (__stdcall * GET_PROC)(char const * , int);
	HINSTANCE hInst = LoadLibraryA("flhash.dll");
	GET_PROC pGetBuf;
	pGetBuf = ((GET_PROC)GetProcAddress(hInst, "flhash"));
	if(!pGetBuf)
		return;
	FILE *test = fopen("FLHash.ini", "r");
	if(test)
	{
		fclose (test);
	}
	else
	{
        FILE *fr = fopen("FLHash.ini", "at");
	    if(fr!=NULL)
		{
		    foreach(lstFLPaths, INISECTIONVALUE, path)
			{
				string line;
				string fpath = Trim(GetParam(path->scValue, ';', 0));
				ifstream readhash(("..\\data\\" + fpath).c_str());
				if(readhash.is_open())
				{
					while(readhash.good())
					{
						getline (readhash,line);
						if(line.find("nickname")==0)
						{

							string temp = Trim(GetParam(line, '=', 1));
							uint ids = pGetBuf(temp.c_str(), temp.length());
							const GoodInfo *gp = GoodList::find_by_id(ids);
							if(!gp)
		                    continue;
							fprintf(fr,"%u = %s = %s\n",ids,temp.c_str(),HkGetStringFromIDS(gp->iIDS).c_str());
						}	
					}
					readhash.close();
				}
				if(fpath.find("universe")==0)
				{
					string uline;
					string universe = Trim(GetParam(path->scValue, ';', 0));
				    ifstream readuinpath(("..\\data\\" + universe).c_str());
					if(readuinpath.is_open())
					{
					    while(readuinpath.good())
						{
							getline (readuinpath,uline);
							if(uline.find("file")==0)
							{
								string uinline;
								string getunipath = Trim(GetParam(uline, '=', 1));
								ifstream readunihash(("..\\data\\universe\\" + getunipath).c_str());
								if(readunihash.is_open())
								{
								    while(readunihash.good())
									{
									    getline (readunihash,uinline);
									    if(uinline.find("nickname")==0)
										{
										    string temp = Trim(GetParam(uinline, '=', 1));
											uint ids = pGetBuf(temp.c_str(), temp.length());
							                fprintf(fr,"%u = %s\n",ids,temp.c_str());
										}
									}
								}
								readunihash.close();
							}
						}
						readuinpath.close();
					}

				}

			}
		}
		fclose (fr);
	}
	FreeLibrary( hInst );
}

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
	sprintf(szBuf, "%u", i);
	return szBuf;
}

string HkGetStringFromIDS(uint iIDS) //Only works for names
{
	if(!iIDS)
		return "";

	uint iDLL = iIDS / 0x10000;
	iIDS -= iDLL * 0x10000;

	char szBuf[512];
	if(LoadStringA(vDLLs[iDLL], iIDS, szBuf, 512))
		return szBuf;
	return "";
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

EXPORT void BaseDestroyed(uint iObject, uint iClientIDBy)
{
	returncode = NOFUNCTIONCALL;
	uint iID;
	pub::SpaceObj::GetDockingTarget(iObject, iID);
	Universe::IBase *base = Universe::get_base(iID);

	char *szBaseName = "";
	if(base)
	{
		__asm
		{
			pushad
			mov ecx, [base]
			mov eax, [base]
			mov eax, [eax]
			call [eax+4]
			mov [szBaseName], eax
			popad
		}
	}
	if(iID == 0)
	{
		ProcessEvent(L"basedestroy basename=%s basehash=%u solarhash=%u by=%s",
					stows(HashName(iObject)).c_str(),
					iObject,
					iID,
					(wchar_t*)Players.GetActiveCharacterName(iClientIDBy));
	}
	else
	{

	    ProcessEvent(L"basedestroy basename=%s basehash=%u solarhash=%u by=%s",
					stows(szBaseName).c_str(),
					iObject,
					iID,
					(wchar_t*)Players.GetActiveCharacterName(iClientIDBy));
	}

}

EXPORT void __stdcall HkCb_AddDmgEntry_AFTER(DamageList *dmgList, unsigned short p1, float p2, enum DamageEntry::SubObjFate p3)
{
	returncode = DEFAULT_RETURNCODE;

    if(p2 == 0 && p1 == 1)
	{
		uint iType;
		pub::SpaceObj::GetType(iDmgToSpaceID,iType);
		uint iClientIDKiller = HkGetClientIDByShip(dmgList->get_inflictor_id());
		if(iClientIDKiller && iType & (OBJ_AIRLOCK_GATE | OBJ_TRADELANE_RING | OBJ_DOCKING_RING | OBJ_STATION | OBJ_JUMP_GATE | OBJ_WEAPONS_PLATFORM | OBJ_SATELLITE | OBJ_DESTROYABLE_DEPOT))
			BaseDestroyed(iDmgToSpaceID, iClientIDKiller);
	}
}

EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO *p_PI = new PLUGIN_INFO();
	p_PI->sName = "BaseHash plugin by kosacid";
	p_PI->sShortName = "basehash";
	p_PI->bMayPause = false;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&LoadSettings, PLUGIN_LoadSettings,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&BaseDestroyed, PLUGIN_BaseDestroyed,0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkCb_AddDmgEntry_AFTER, PLUGIN_HkCb_AddDmgEntry_AFTER,0));
	
	return p_PI;
}