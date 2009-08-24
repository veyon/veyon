#include "stdhdrs.h"
#include "inifile.h"
#include <string>
#include <algorithm>
#include <cctype>

//  We first use shellexecute with "runas"
//  This way we can use UAC and user/passwd
//	Runas is standard OS, so no security risk

const char winvncSettings[]				= "-settings";
const char winvncStopservice[]			= "-stopservice";
const char winvncStartservice[]			= "-startservice";
const char winvncInstallService[]		= "-install";
const char winvncUnInstallService[]		= "-uninstall";
const char winvncSecurityEditor[]		= "-securityeditor";

extern char service_name[];

/////////////////////////////////////////////////////////////////////
void
Set_settings_as_admin(char *mycommand)
{
	char exe_file_name[MAX_PATH];
	char commanline[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	strcpy(commanline, winvncSettings);
	strcat(commanline, ":");
	strcat(commanline, mycommand);

	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = commanline;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}


void Copy_to_Secure_from_temp_helper(char *lpCmdLine)
{
IniFile myIniFile_In;
IniFile myIniFile_Out;
myIniFile_Out.IniFileSetSecure();
myIniFile_In.IniFileSetTemp(lpCmdLine);

TCHAR *group1=new char[150];
TCHAR *group2=new char[150];
TCHAR *group3=new char[150];
BOOL BUseRegistry;
LONG MSLogonRequired;
LONG NewMSLogon;
LONG locdom1;
LONG locdom2;
LONG locdom3;
LONG DebugMode=2;
LONG Avilog=0;
LONG DebugLevel=10;
LONG DisableTrayIcon;
LONG LoopbackOnly;
LONG UseDSMPlugin;
LONG AllowLoopback;
LONG AuthRequired;
LONG ConnectPriority;

char DSMPlugin[128];
char *authhosts=new char[150];

LONG AllowShutdown=1;
LONG AllowProperties=1;
LONG AllowEditClients=1;

LONG FileTransferEnabled;
LONG FTUserImpersonation;
LONG BlankMonitorEnabled=1;
LONG DefaultScale=1;
LONG CaptureAlphaBlending=1;
LONG BlackAlphaBlending=1;

LONG SocketConnect=1;
LONG HTTPConnect;
LONG XDMCPConnect;
LONG AutoPortSelect=1;
LONG PortNumber;
LONG HttpPortNumber;
LONG IdleTimeout;

LONG RemoveWallpaper=1;
LONG RemoveAero=1;

LONG QuerySetting=1;
LONG QueryTimeout=10;
LONG QueryAccept;
LONG QueryIfNoLogon;

LONG EnableRemoteInputs=1;
LONG LockSettings=0;
LONG DisableLocalInputs=0;
LONG EnableJapInput=0;
LONG kickrdp=0;
LONG clearconsole=0;

#define MAXPWLEN 8
char passwd[MAXPWLEN];

LONG TurboMode=1;
LONG PollUnderCursor=0;
LONG PollForeground=1;
LONG PollFullScreen=1;
LONG PollConsoleOnly=0;
LONG PollOnEventOnly=0;
LONG Driver=0;
LONG Hook=1;
LONG Virtual;
LONG SingleWindow=0;
char SingleWindowName[32];
char path[512];

LONG Primary=1;
LONG Secundary=0;
LONG MaxCpu=40;


BUseRegistry = myIniFile_In.ReadInt("admin", "UseRegistry", 0);
if (!myIniFile_Out.WriteInt("admin", "UseRegistry", BUseRegistry))
{
		//error
		DWORD error=GetLastError();
		MessageBox(NULL,"Permission denied:Uncheck [_] Protect my computer... in run as dialog or use user with write permission." ,myIniFile_Out.myInifile,MB_ICONERROR);
}

MSLogonRequired=myIniFile_In.ReadInt("admin", "MSLogonRequired", false);
myIniFile_Out.WriteInt("admin", "MSLogonRequired", MSLogonRequired);
NewMSLogon=myIniFile_In.ReadInt("admin", "NewMSLogon", false);
myIniFile_Out.WriteInt("admin", "NewMSLogon", NewMSLogon);


myIniFile_In.ReadString("admin_auth","group1",group1,150);
myIniFile_In.ReadString("admin_auth","group2",group2,150);
myIniFile_In.ReadString("admin_auth","group3",group3,150);
myIniFile_Out.WriteString("admin_auth", "group1",group1);
myIniFile_Out.WriteString("admin_auth", "group2",group2);
myIniFile_Out.WriteString("admin_auth", "group3",group3);



locdom1=myIniFile_In.ReadInt("admin_auth", "locdom1",0);
locdom2=myIniFile_In.ReadInt("admin_auth", "locdom2",0);
locdom3=myIniFile_In.ReadInt("admin_auth", "locdom3",0);
myIniFile_Out.WriteInt("admin_auth", "locdom1", locdom1);
myIniFile_Out.WriteInt("admin_auth", "locdom2", locdom2);
myIniFile_Out.WriteInt("admin_auth", "locdom3", locdom3);


DebugMode=myIniFile_In.ReadInt("admin", "DebugMode", 0);
Avilog=myIniFile_In.ReadInt("admin", "Avilog", 0);
myIniFile_In.ReadString("admin", "path", path,512);
DebugLevel=myIniFile_In.ReadInt("admin", "DebugLevel", 0);
DisableTrayIcon=myIniFile_In.ReadInt("admin", "DisableTrayIcon", false);
LoopbackOnly=myIniFile_In.ReadInt("admin", "LoopbackOnly", false);

myIniFile_Out.WriteInt("admin", "DebugMode", DebugMode);
myIniFile_Out.WriteInt("admin", "Avilog", Avilog);
myIniFile_Out.WriteString("admin", "path", path);
myIniFile_Out.WriteInt("admin", "DebugLevel", DebugLevel);
myIniFile_Out.WriteInt("admin", "DisableTrayIcon", DisableTrayIcon);
myIniFile_Out.WriteInt("admin", "LoopbackOnly", LoopbackOnly);

UseDSMPlugin=myIniFile_In.ReadInt("admin", "UseDSMPlugin", false);
AllowLoopback=myIniFile_In.ReadInt("admin", "AllowLoopback", false);
AuthRequired=myIniFile_In.ReadInt("admin", "AuthRequired", true);
ConnectPriority=myIniFile_In.ReadInt("admin", "ConnectPriority", 0);

myIniFile_Out.WriteInt("admin", "UseDSMPlugin", UseDSMPlugin);
myIniFile_Out.WriteInt("admin", "AllowLoopback", AllowLoopback);
myIniFile_Out.WriteInt("admin", "AuthRequired", AuthRequired);
myIniFile_Out.WriteInt("admin", "ConnectPriority", ConnectPriority);


myIniFile_In.ReadString("admin", "DSMPlugin",DSMPlugin,128);
myIniFile_In.ReadString("admin", "AuthHosts",authhosts,150);

myIniFile_Out.WriteString("admin", "DSMPlugin",DSMPlugin);
myIniFile_Out.WriteString("admin", "AuthHosts",authhosts);

AllowShutdown=myIniFile_In.ReadInt("admin", "AllowShutdown", true);
AllowProperties=myIniFile_In.ReadInt("admin", "AllowProperties", true);
AllowEditClients=myIniFile_In.ReadInt("admin", "AllowEditClients", true);
myIniFile_Out.WriteInt("admin", "AllowShutdown" ,AllowShutdown);
myIniFile_Out.WriteInt("admin", "AllowProperties" ,AllowProperties);
myIniFile_Out.WriteInt("admin", "AllowEditClients" ,AllowEditClients);


FileTransferEnabled=myIniFile_In.ReadInt("admin", "FileTransferEnabled", true);
FTUserImpersonation=myIniFile_In.ReadInt("admin", "FTUserImpersonation", true);
BlankMonitorEnabled = myIniFile_In.ReadInt("admin", "BlankMonitorEnabled", true);
DefaultScale = myIniFile_In.ReadInt("admin", "DefaultScale", 1);
CaptureAlphaBlending = myIniFile_In.ReadInt("admin", "CaptureAlphaBlending", false); // sf@2005
BlackAlphaBlending = myIniFile_In.ReadInt("admin", "BlackAlphaBlending", false); // sf@2005

Primary = myIniFile_In.ReadInt("admin", "primary", true);
Secundary = myIniFile_In.ReadInt("admin", "secundary", false);

myIniFile_Out.WriteInt("admin", "FileTransferEnabled", FileTransferEnabled);
myIniFile_Out.WriteInt("admin", "FTUserImpersonation", FTUserImpersonation);
myIniFile_Out.WriteInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
myIniFile_Out.WriteInt("admin", "DefaultScale", DefaultScale);
myIniFile_Out.WriteInt("admin", "CaptureAlphaBlending", CaptureAlphaBlending);
myIniFile_Out.WriteInt("admin", "BlackAlphaBlending", BlackAlphaBlending);

myIniFile_Out.WriteInt("admin", "primary", Primary);
myIniFile_Out.WriteInt("admin", "secundary", Secundary);


	// Connection prefs
SocketConnect=myIniFile_In.ReadInt("admin", "SocketConnect", true);
HTTPConnect=myIniFile_In.ReadInt("admin", "HTTPConnect", true);
XDMCPConnect=myIniFile_In.ReadInt("admin", "XDMCPConnect", true);
AutoPortSelect=myIniFile_In.ReadInt("admin", "AutoPortSelect", true);
PortNumber=myIniFile_In.ReadInt("admin", "PortNumber", 0);
HttpPortNumber=myIniFile_In.ReadInt("admin", "HTTPPortNumber",0);
IdleTimeout=myIniFile_In.ReadInt("admin", "IdleTimeout", 0);
myIniFile_Out.WriteInt("admin", "SocketConnect", SocketConnect);
myIniFile_Out.WriteInt("admin", "HTTPConnect", HTTPConnect);
myIniFile_Out.WriteInt("admin", "XDMCPConnect", XDMCPConnect);
myIniFile_Out.WriteInt("admin", "AutoPortSelect", AutoPortSelect);
myIniFile_Out.WriteInt("admin", "PortNumber", PortNumber);
myIniFile_Out.WriteInt("admin", "HTTPPortNumber", HttpPortNumber);
myIniFile_Out.WriteInt("admin", "IdleTimeout", IdleTimeout);
	
RemoveWallpaper=myIniFile_In.ReadInt("admin", "RemoveWallpaper", 0);
RemoveAero=myIniFile_In.ReadInt("admin", "RemoveAero", 0);
myIniFile_Out.WriteInt("admin", "RemoveWallpaper", RemoveWallpaper);
myIniFile_Out.WriteInt("admin", "RemoveAero", RemoveAero);

	// Connection querying settings
QuerySetting=myIniFile_In.ReadInt("admin", "QuerySetting", 0);
QueryTimeout=myIniFile_In.ReadInt("admin", "QueryTimeout", 0);
QueryAccept=myIniFile_In.ReadInt("admin", "QueryAccept", 0);
QueryIfNoLogon=myIniFile_In.ReadInt("admin", "QueryIfNoLogon", 0);
myIniFile_Out.WriteInt("admin", "QuerySetting", QuerySetting);
myIniFile_Out.WriteInt("admin", "QueryTimeout", QueryTimeout);
myIniFile_Out.WriteInt("admin", "QueryAccept", QueryAccept);
myIniFile_Out.WriteInt("admin", "QueryIfNoLogon", QueryIfNoLogon);

myIniFile_In.ReadPassword(passwd,MAXPWLEN);
myIniFile_Out.WritePassword(passwd);

EnableRemoteInputs=myIniFile_In.ReadInt("admin", "InputsEnabled", 0);
LockSettings=myIniFile_In.ReadInt("admin", "LockSetting", 0);
DisableLocalInputs=myIniFile_In.ReadInt("admin", "LocalInputsDisabled", 0);
EnableJapInput=myIniFile_In.ReadInt("admin", "EnableJapInput", 0);
kickrdp=myIniFile_In.ReadInt("admin", "kickrdp", 0);
clearconsole=myIniFile_In.ReadInt("admin", "clearconsole", 0);

myIniFile_Out.WriteInt("admin", "InputsEnabled", EnableRemoteInputs);
myIniFile_Out.WriteInt("admin", "LockSetting", LockSettings);
myIniFile_Out.WriteInt("admin", "LocalInputsDisabled", DisableLocalInputs);	
myIniFile_Out.WriteInt("admin", "EnableJapInput", EnableJapInput);	
myIniFile_Out.WriteInt("admin", "kickrdp", kickrdp);
myIniFile_Out.WriteInt("admin", "clearconsole", clearconsole);



TurboMode = myIniFile_In.ReadInt("poll", "TurboMode", 0);
PollUnderCursor=myIniFile_In.ReadInt("poll", "PollUnderCursor", 0);
PollForeground=myIniFile_In.ReadInt("poll", "PollForeground", 0);
PollFullScreen=myIniFile_In.ReadInt("poll", "PollFullScreen", 0);
PollConsoleOnly=myIniFile_In.ReadInt("poll", "OnlyPollConsole", 0);
PollOnEventOnly=myIniFile_In.ReadInt("poll", "OnlyPollOnEvent", 0);
MaxCpu=myIniFile_In.ReadInt("poll", "MaxCpu", 0);
Driver=myIniFile_In.ReadInt("poll", "EnableDriver", 0);
Hook=myIniFile_In.ReadInt("poll", "EnableHook", 0);
Virtual=myIniFile_In.ReadInt("poll", "EnableVirtual", 0);

SingleWindow=myIniFile_In.ReadInt("poll","SingleWindow",SingleWindow);
myIniFile_In.ReadString("poll", "SingleWindowName", SingleWindowName,32);

myIniFile_Out.WriteInt("poll", "TurboMode", TurboMode);
myIniFile_Out.WriteInt("poll", "PollUnderCursor", PollUnderCursor);
myIniFile_Out.WriteInt("poll", "PollForeground", PollForeground);
myIniFile_Out.WriteInt("poll", "PollFullScreen", PollFullScreen);
myIniFile_Out.WriteInt("poll", "OnlyPollConsole",PollConsoleOnly);
myIniFile_Out.WriteInt("poll", "OnlyPollOnEvent", PollOnEventOnly);
myIniFile_Out.WriteInt("poll", "MaxCpu", MaxCpu);
myIniFile_Out.WriteInt("poll", "EnableDriver", Driver);
myIniFile_Out.WriteInt("poll", "EnableHook", Hook);
myIniFile_Out.WriteInt("poll", "EnableVirtual", Virtual);

myIniFile_Out.WriteInt("poll", "SingleWindow", SingleWindow);
myIniFile_Out.WriteString("poll", "SingleWindowName", SingleWindowName);

DeleteFile(lpCmdLine);
}
void
Real_settings(char *mycommand)
{
Copy_to_Secure_from_temp_helper(mycommand);
}

void
Set_stop_service_as_admin()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	SHELLEXECUTEINFO shExecInfo;

	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = winvncStopservice;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);

}

void
Real_stop_service()
{
    char command[MAX_PATH + 32]; // 29 January 2008 jdp 
    _snprintf(command, sizeof command, "net stop \"%s\"", service_name);
	WinExec(command,SW_HIDE);
}

void
Set_start_service_as_admin()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	SHELLEXECUTEINFO shExecInfo;

	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = winvncStartservice;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);

}

void
Real_start_service()
{
    char command[MAX_PATH + 32]; // 29 January 2008 jdp 
    _snprintf(command, sizeof command, "net start \"%s\"", service_name);
	WinExec(command,SW_HIDE);
}

void
Set_install_service_as_admin()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	SHELLEXECUTEINFO shExecInfo;

	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = winvncInstallService;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);

}

void
Set_uninstall_service_as_admin()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	SHELLEXECUTEINFO shExecInfo;

	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = winvncUnInstallService;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);

}

void
winvncSecurityEditorHelper_as_admin()
{
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);

	SHELLEXECUTEINFO shExecInfo;

	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = winvncSecurityEditor;
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}
void make_upper(std::string& str)
{
    // convert to uppercase
    std::transform(str.begin(), str.end(), str.begin(), toupper);//(int(*)(int))
}
//**************************************************************************
// GetServiceName() looks up service by application path. If found, the function
// fills pszServiceName (must be at least 256+1 characters long).
bool GetServiceName(TCHAR *pszAppPath, TCHAR *pszServiceName)
{
    // prepare given application path for matching against service list
    std::string appPath(pszAppPath);
    // convert to uppercase
    make_upper(appPath);

	// connect to serice control manager
    SC_HANDLE hManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hManager)
        return false;

    DWORD dwBufferSize = 0;
    DWORD dwCount = 0;
    DWORD dwPosition = 0;
    bool bResult = false;

    // call EnumServicesStatus() the first time to receive services array size
    BOOL bOK = EnumServicesStatus(
        hManager,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        NULL,
        0,
        &dwBufferSize,
        &dwCount,
        &dwPosition);
    if (!bOK && GetLastError() == ERROR_MORE_DATA)
    {
        // allocate space per results from the first call
        ENUM_SERVICE_STATUS *pServices = (ENUM_SERVICE_STATUS *) new UCHAR[dwBufferSize];
        if (pServices)
        {
            // call EnumServicesStatus() the second time to actually get the services array
            bOK = EnumServicesStatus(
                hManager,
                SERVICE_WIN32,
                SERVICE_STATE_ALL,
                pServices,
                dwBufferSize,
                &dwBufferSize,
                &dwCount,
                &dwPosition);
            if (bOK)
            {
                // iterate through all services returned by EnumServicesStatus()
                for (DWORD i = 0; i < dwCount && !bResult; i++)
                {
                    // open service
                    SC_HANDLE hService = OpenService(hManager,
                        pServices[i].lpServiceName,
                        GENERIC_READ);
                    if (!hService)
                        break;

                    // call QueryServiceConfig() the first time to receive buffer size
                    bOK = QueryServiceConfig(
                        hService,
                        NULL,
                        0,
                        &dwBufferSize);
                    if (!bOK && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        // allocate space per results from the first call
                        QUERY_SERVICE_CONFIG *pServiceConfig = (QUERY_SERVICE_CONFIG *) new UCHAR[dwBufferSize];
                        if (pServiceConfig)
                        {
                            // call EnumServicesStatus() the second time to actually get service config
                            bOK = QueryServiceConfig(
                                hService,
                                pServiceConfig,
                                dwBufferSize,
                                &dwBufferSize);
                            if (bOK)
                            {
                                // match given application name against executable path in the service config
                                std::string servicePath(pServiceConfig->lpBinaryPathName);
                                make_upper(servicePath);
                                if (servicePath.find(appPath.c_str()) != -1)
                                {
                                    bResult = true;
                                    strncpy(pszServiceName, pServices[i].lpServiceName, 256);
                                    pszServiceName[255] = 0;
                                }
                            }

                            delete [] (UCHAR *) pServiceConfig;
                        }
                    }

                    CloseServiceHandle(hService);
                }
            }

            delete [] (UCHAR *) pServices;
        }
    }

    // disconnect from serice control manager
    CloseServiceHandle(hManager);

    return bResult;
}
