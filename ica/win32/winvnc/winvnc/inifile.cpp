#include "stdhdrs.h"
#include "inifile.h"
void Set_settings_as_admin(char *mycommand);


IniFile::IniFile()
{
char WORKDIR[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return;
		*p = '\0';
		}
	strcpy(myInifile,"");
	strcat(myInifile,WORKDIR);//set the directory
	strcat(myInifile,"\\");
	strcat(myInifile,INIFILE_NAME);
}

void
IniFile::IniFileSetSecure()
{
char WORKDIR[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return;
		*p = '\0';
		}
	strcpy(myInifile,"");
	strcat(myInifile,WORKDIR);//set the directory
	strcat(myInifile,"\\");
	strcat(myInifile,INIFILE_NAME);
}

void
IniFile::IniFileSetTemp(char *lpCmdLine)
{	
	strcpy(myInifile,lpCmdLine);
}

/*void
IniFile::IniFileSetTemp()
{
char WORKDIR[MAX_PATH];

	if (!GetTempPath(MAX_PATH,WORKDIR))
	{
		//Function failed, just set something
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return;
		*p = '\0';
		}
		strcpy(myInifile,"");
		strcat(myInifile,WORKDIR);//set the directory
		strcat(myInifile,"\\");
		strcat(myInifile,INIFILE_NAME);
		return;
	}

	strcpy(myInifile,"");
	strcat(myInifile,WORKDIR);//set the directory
	strcat(myInifile,INIFILE_NAME);
}*/

void
IniFile::copy_to_secure()
{
		char dir[MAX_PATH];

		char exe_file_name[MAX_PATH];
		GetModuleFileName(0, exe_file_name, MAX_PATH);

		strcpy(dir, exe_file_name);
		strcat(dir, " -settingshelper");
		strcat(dir, ":");
		strcat(dir, myInifile);


		STARTUPINFO          StartUPInfo;
		PROCESS_INFORMATION  ProcessInfo;
		HANDLE Token=NULL;
		HANDLE process=NULL;
		ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
		ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
		StartUPInfo.wShowWindow = SW_SHOW;
		StartUPInfo.lpDesktop = "Winsta0\\Default";
		StartUPInfo.cb = sizeof(STARTUPINFO);
		HWND tray = FindWindow(("Shell_TrayWnd"), 0);
		if (!tray)
			return;
	
		DWORD processId = 0;
			GetWindowThreadProcessId(tray, &processId);
		if (!processId)
			return;	
		process = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
		if (!process)
			return;	
		OpenProcessToken(process, MAXIMUM_ALLOWED, &Token);
		CreateProcessAsUser(Token,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
		DWORD error=GetLastError();
		if (process) CloseHandle(process);
		if (Token) CloseHandle(Token);
		if (error==1314)
		{
			Set_settings_as_admin(myInifile);
		}

}

IniFile::~IniFile()
{
}

bool
IniFile::WriteString(char *key1, char *key2,char *value)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile); 
	return (FALSE != WritePrivateProfileString(key1,key2, value,myInifile));
}

bool 
IniFile::WriteInt(char *key1, char *key2,int value)
{
	char       buf[32];
	wsprintf(buf, "%d", value);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	int result=WritePrivateProfileString(key1,key2, buf,myInifile);
	if (result==0) return false;
	return true;
}

int
IniFile::ReadInt(char *key1, char *key2,int Defaultvalue)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	return GetPrivateProfileInt(key1, key2, Defaultvalue, myInifile);
}

void 
IniFile::ReadString(char *key1, char *key2,char *value,int valuesize)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	GetPrivateProfileString(key1,key2, "",value,valuesize,myInifile);
}

void 
IniFile::ReadPassword(char *value,int valuesize)
{
	//int size=ReadInt("ultravnc", "passwdsize",0);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifilePasswd);
	GetPrivateProfileStruct("ultravnc","passwd",value,8,myInifile);
}

void //PGM
IniFile::ReadPassword2(char *value,int valuesize) //PGM
{ //PGM
	GetPrivateProfileStruct("ultravnc","passwd2",value,8,myInifile); //PGM
} //PGM

bool
IniFile::WritePassword(char *value)
{
		//WriteInt("ultravnc", "passwdsize",sizeof(value));
		//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
		return (FALSE != WritePrivateProfileStruct("ultravnc","passwd", value,8,myInifile));
}

bool //PGM
IniFile::WritePassword2(char *value) //PGM
{ //PGM
		return (FALSE != WritePrivateProfileStruct("ultravnc","passwd2", value,8,myInifile)); //PGM
} //PGM

bool IniFile::IsWritable()
{
    bool writable = WriteInt("Permissions", "isWritable",1);
    if (writable)
        WritePrivateProfileSection("Permissions", "", myInifile);

    return writable;
}

