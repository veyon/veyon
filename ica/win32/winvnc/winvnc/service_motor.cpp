/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#include "stdhdrs.h"
#include <windows.h>
#include <wtsapi32.h>
#include "common/win32_helpers.h"

static void WINAPI service_main(DWORD, LPTSTR *);
static void WINAPI control_handler(DWORD controlCode);
static DWORD WINAPI control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
static int pad();

SERVICE_STATUS serviceStatus;
static SERVICE_STATUS_HANDLE serviceStatusHandle=0;
HANDLE stopServiceEvent=0;
extern HANDLE hEvent;
static char service_path[MAX_PATH];
void monitor_sessions();
void Restore_after_reboot();
char service_name[256]="uvnc_service";
char *app_name = "UltraVNC";
void disconnect_remote_sessions();
char cmdtext[256];
extern int clear_console;
bool IsWin2000()
{
	OSVERSIONINFO OSversion;
	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);

    if (OSversion.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==0)
            return true; 
						
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
static void WINAPI service_main(DWORD argc, LPTSTR* argv) {
    /* initialise service status */
    serviceStatus.dwServiceType=SERVICE_WIN32;
    serviceStatus.dwCurrentState=SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted=0;
    serviceStatus.dwWin32ExitCode=NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode=NO_ERROR;
    serviceStatus.dwCheckPoint=0;
    serviceStatus.dwWaitHint=0;

    typedef SERVICE_STATUS_HANDLE (WINAPI * pfnRegisterServiceCtrlHandlerEx)(LPCTSTR, LPHANDLER_FUNCTION_EX, LPVOID);
    helper::DynamicFn<pfnRegisterServiceCtrlHandlerEx> pRegisterServiceCtrlHandlerEx("advapi32.dll","RegisterServiceCtrlHandlerExA");

    if (pRegisterServiceCtrlHandlerEx.isValid())
      serviceStatusHandle = (*pRegisterServiceCtrlHandlerEx)(service_name, control_handler_ex, 0);
    else 
      serviceStatusHandle = RegisterServiceCtrlHandler(service_name, control_handler);

    if(serviceStatusHandle) {
        /* service is starting */
        serviceStatus.dwCurrentState=SERVICE_START_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

        /* do initialisation here */
        stopServiceEvent=CreateEvent(0, FALSE, FALSE, 0);

        /* running */
        serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
        if (!IsWin2000())
            serviceStatus.dwControlsAccepted |= SERVICE_ACCEPT_SESSIONCHANGE;

        serviceStatus.dwCurrentState=SERVICE_RUNNING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

Restore_after_reboot();
monitor_sessions();

        /* service was stopped */
        serviceStatus.dwCurrentState=SERVICE_STOP_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

        /* do cleanup here */
        if (stopServiceEvent) CloseHandle(stopServiceEvent);
        stopServiceEvent=0;

        /* service is now stopped */
        serviceStatus.dwControlsAccepted&=
            ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
        serviceStatus.dwCurrentState=SERVICE_STOPPED;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
    }
}
////////////////////////////////////////////////////////////////////////////////
static void WINAPI control_handler(DWORD controlCode)
{
  control_handler_ex(controlCode, 0, 0, 0);
}
static DWORD WINAPI control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    switch (controlCode) {
    case SERVICE_CONTROL_INTERROGATE:
        break;

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        serviceStatus.dwCurrentState=SERVICE_STOP_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);
        SetEvent(stopServiceEvent);
		SetEvent(hEvent);
        return NO_ERROR;

    case SERVICE_CONTROL_PAUSE:
        break;

    case SERVICE_CONTROL_CONTINUE:
        break;

    case SERVICE_CONTROL_SESSIONCHANGE:
        {
            if (dwEventType == WTS_REMOTE_DISCONNECT)
            {
                // disconnect rdp, and reconnect to the console
                if ( clear_console) disconnect_remote_sessions();
            }
        }
        break;

    default:
        if(controlCode >= 128 && controlCode <= 255)
            break; 
        else
            break;
    }
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
    return NO_ERROR;
}
////////////////////////////////////////////////////////////////////////////////
int start_service(char *cmd) {
	strcpy_s(cmdtext,256,cmd);
    SERVICE_TABLE_ENTRY serviceTable[]={
	 {service_name, service_main},
        {0, 0}
    };

    if(!StartServiceCtrlDispatcher(serviceTable)) {
        return 1;
    }
    return 0; /* NT service started */
}
////////////////////////////////////////////////////////////////////////////////
void set_service_description()
{
    // Add service description 
	DWORD	dw;
	HKEY hKey;
	char tempName[256];
    char desc[] = "Provides secure remote desktop sharing";
	_snprintf(tempName,  sizeof tempName, "SYSTEM\\CurrentControlSet\\Services\\%s", service_name);
	RegCreateKeyEx(HKEY_LOCAL_MACHINE,
						tempName,
						0,
						REG_NONE,
						REG_OPTION_NON_VOLATILE,
						KEY_READ|KEY_WRITE,
						NULL,
						&hKey,
						&dw);
	RegSetValueEx(hKey,
					"Description",
					0,
					REG_SZ,
					(const BYTE *)desc,
					strlen(desc)+1);


	RegCloseKey(hKey);
}


// List of other required services ("dependency 1\0dependency 2\0\0")
// *** These need filling in properly
#define VNCDEPENDENCIES    "Tcpip\0\0"



int install_service(void) {
    SC_HANDLE scm, service;
	pad();

    scm=OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
    if(!scm) {
        MessageBoxSecure(NULL, "Failed to open service control manager",
            app_name, MB_ICONERROR);
        return 1;
    }
    //"Provides secure remote desktop sharing"
    service=CreateService(scm,service_name, service_name, SERVICE_ALL_ACCESS,
                          SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, service_path,
        NULL, NULL, VNCDEPENDENCIES, NULL, NULL);
    if(!service) {
		DWORD myerror=GetLastError();
		if (myerror==ERROR_ACCESS_DENIED)
		{
			MessageBoxSecure(NULL, "Failed: Permission denied",
            app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror==ERROR_SERVICE_EXISTS)
		{
			//MessageBoxSecure(NULL, "Failed: Already exist",
            //"UltraVnc", MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}

        MessageBoxSecure(NULL, "Failed to create a new service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(scm);
        return 1;
    }
    else
        set_service_description();
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
int uninstall_service(void) {
    SC_HANDLE scm, service;
    SERVICE_STATUS serviceStatus;

    scm=OpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if(!scm) {
        MessageBoxSecure(NULL, "Failed to open service control manager",
            app_name, MB_ICONERROR);
        return 1;
    }

	service=OpenService(scm, service_name,
        SERVICE_QUERY_STATUS | DELETE);
    if(!service) {
		DWORD myerror=GetLastError();
		if (myerror==ERROR_ACCESS_DENIED)
		{
			MessageBoxSecure(NULL, "Failed: Permission denied",
            app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror==ERROR_SERVICE_DOES_NOT_EXIST)
		{
#if 0
			MessageBoxSecure(NULL, "Failed: Service is not installed",
            app_name, MB_ICONERROR);
#endif
			CloseServiceHandle(scm);
			return 1;
		}

        MessageBoxSecure(NULL, "Failed to open the service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(scm);
        return 1;
    }
    if(!QueryServiceStatus(service, &serviceStatus)) {
        MessageBoxSecure(NULL, "Failed to query service status",
            app_name, MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return 1;
    }
    if(serviceStatus.dwCurrentState!=SERVICE_STOPPED) {
        //MessageBoxSecure(NULL, "The service is still running, disable it first",
        //    "UltraVnc", MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
		Sleep(2500);uninstall_service();
        return 1;
    }
    if(!DeleteService(service)) {
        MessageBoxSecure(NULL, "Failed to delete the service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return 1;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////
static int pad()
{
	char exe_file_name[MAX_PATH], dir[MAX_PATH], *ptr;
    GetModuleFileName(0, exe_file_name, MAX_PATH);

    /* set current directory */
    strcpy(dir, exe_file_name);
    ptr=strrchr(dir, '\\'); /* last backslash */
    if(ptr)
        ptr[1]='\0'; /* truncate program name */
    if(!SetCurrentDirectory(dir)) {
        return 1;
    }

    strcpy(service_path, "\"");
    strcat(service_path, exe_file_name);
    strcat(service_path, "\" -service");
	return 0;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
BOOL CreateServiceSafeBootKey()
{
	HKEY hKey;
	DWORD dwDisp = 0;
	LONG lSuccess;
	char szKey[1024];
	_snprintf(szKey, 1024, "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\%s\\%s", "Network", service_name);
	lSuccess = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKey, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
	if (lSuccess == ERROR_SUCCESS)
	{
    RegSetValueEx(hKey, NULL, 0, REG_SZ, (unsigned char*)"Service", 8);
		RegCloseKey(hKey);
		return TRUE;
	}
	else
		return FALSE;
}
////////////////////////////////////////////////////////////////////////////////
void Set_Safemode()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6)
			{
					char drivepath[150];
					char systemdrive[150];
					char stringvalue[512];
					GetEnvironmentVariable("SYSTEMDRIVE", systemdrive, 150);
					strcat (systemdrive,"/boot.ini");
					GetPrivateProfileString("boot loader","default","",drivepath,150,systemdrive);
					if (strlen(drivepath)==0) return;
					GetPrivateProfileString("operating systems",drivepath,"",stringvalue,512,systemdrive);
					if (strlen(stringvalue)==0) return;
					strcat(stringvalue," /safeboot:network");
					SetFileAttributes(systemdrive,FILE_ATTRIBUTE_NORMAL);
					WritePrivateProfileString("operating systems",drivepath,stringvalue,systemdrive);

			}
			else
			{

#ifdef _X64
			char systemroot[150];
			GetEnvironmentVariable("SystemRoot", systemroot, 150);
			char exe_file_name[MAX_PATH];
			char parameters[MAX_PATH];
			strcpy(exe_file_name,systemroot);
			strcat(exe_file_name,"\\system32\\");
			strcat(exe_file_name,"bcdedit.exe");
			strcpy(parameters,"/set safeboot network");
			SHELLEXECUTEINFO shExecInfo;
			shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			shExecInfo.fMask = 0;
			shExecInfo.hwnd = GetForegroundWindow();
			shExecInfo.lpVerb = "runas";
			shExecInfo.lpFile = exe_file_name;
			shExecInfo.lpParameters = parameters;
			shExecInfo.lpDirectory = NULL;
			shExecInfo.nShow = SW_HIDE;
			shExecInfo.hInstApp = NULL;
			ShellExecuteEx(&shExecInfo);
#else

			typedef BOOL (WINAPI *LPFN_Wow64DisableWow64FsRedirection)(PVOID* OldValue);
			typedef BOOL (WINAPI *LPFN_Wow64RevertWow64FsRedirection)(PVOID OldValue);
			PVOID OldValue;  
			LPFN_Wow64DisableWow64FsRedirection pfnWow64DisableWowFsRedirection = (LPFN_Wow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64DisableWow64FsRedirection");
			LPFN_Wow64RevertWow64FsRedirection pfnWow64RevertWow64FsRedirection = (LPFN_Wow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64RevertWow64FsRedirection");
			if (pfnWow64DisableWowFsRedirection && pfnWow64RevertWow64FsRedirection) 
			{
				if(TRUE == pfnWow64DisableWowFsRedirection(&OldValue))
					{
						char systemroot[150];
						GetEnvironmentVariable("SystemRoot", systemroot, 150);
						char exe_file_name[MAX_PATH];
						char parameters[MAX_PATH];
						strcpy(exe_file_name,systemroot);
						strcat(exe_file_name,"\\system32\\");
						strcat(exe_file_name,"bcdedit.exe");
						strcpy(parameters,"/set safeboot network");
						SHELLEXECUTEINFO shExecInfo;
						shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						shExecInfo.fMask = 0;
						shExecInfo.hwnd = GetForegroundWindow();
						shExecInfo.lpVerb = "runas";
						shExecInfo.lpFile = exe_file_name;
						shExecInfo.lpParameters = parameters;
						shExecInfo.lpDirectory = NULL;
						shExecInfo.nShow = SW_HIDE;
						shExecInfo.hInstApp = NULL;
						ShellExecuteEx(&shExecInfo);
						pfnWow64RevertWow64FsRedirection(OldValue);
					}
				else
				{
					char systemroot[150];
					GetEnvironmentVariable("SystemRoot", systemroot, 150);
					char exe_file_name[MAX_PATH];
					char parameters[MAX_PATH];
					strcpy(exe_file_name,systemroot);
					strcat(exe_file_name,"\\system32\\");
					strcat(exe_file_name,"bcdedit.exe");
					strcpy(parameters,"/set safeboot network");
					SHELLEXECUTEINFO shExecInfo;
					shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
					shExecInfo.fMask = 0;
					shExecInfo.hwnd = GetForegroundWindow();
					shExecInfo.lpVerb = "runas";
					shExecInfo.lpFile = exe_file_name;
					shExecInfo.lpParameters = parameters;
					shExecInfo.lpDirectory = NULL;
					shExecInfo.nShow = SW_HIDE;
					shExecInfo.hInstApp = NULL;
					ShellExecuteEx(&shExecInfo);
				}
			}
			else
			{
				char systemroot[150];
				GetEnvironmentVariable("SystemRoot", systemroot, 150);
				char exe_file_name[MAX_PATH];
				char parameters[MAX_PATH];
				strcpy(exe_file_name,systemroot);
				strcat(exe_file_name,"\\system32\\");
				strcat(exe_file_name,"bcdedit.exe");
				strcpy(parameters,"/set safeboot network");
				SHELLEXECUTEINFO shExecInfo;
				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				shExecInfo.fMask = 0;
				shExecInfo.hwnd = GetForegroundWindow();
				shExecInfo.lpVerb = "runas";
				shExecInfo.lpFile = exe_file_name;
				shExecInfo.lpParameters = parameters;
				shExecInfo.lpDirectory = NULL;
				shExecInfo.nShow = SW_HIDE;
				shExecInfo.hInstApp = NULL;
				ShellExecuteEx(&shExecInfo);
			}
#endif

			}
}

BOOL reboot()
{
	HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    if (OpenProcessToken(    GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                & hToken)) 
		{
			LookupPrivilegeValue(    NULL,  SE_SHUTDOWN_NAME,  & tkp.Privileges[0].Luid);          
			tkp.PrivilegeCount = 1; 
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			if(AdjustTokenPrivileges(    hToken,  FALSE,  & tkp,  0,  (PTOKEN_PRIVILEGES)NULL,  0))
				{
					ExitWindowsEx(EWX_REBOOT, 0);
				}
		}
	return TRUE;
}

BOOL Force_reboot()
{
	HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
    if (OpenProcessToken(    GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
                & hToken)) 
		{
			LookupPrivilegeValue(    NULL,  SE_SHUTDOWN_NAME,  & tkp.Privileges[0].Luid);          
			tkp.PrivilegeCount = 1; 
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			if(AdjustTokenPrivileges(    hToken,  FALSE,  & tkp,  0,  (PTOKEN_PRIVILEGES)NULL,  0))
				{
					OSVERSIONINFO OSversion;	
					OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
					GetVersionEx(&OSversion);
					if(OSversion.dwMajorVersion<6)
					{
					ExitWindowsEx(EWX_REBOOT|EWX_FORCEIFHUNG, 0);
					}
					else
					{
					ExitWindowsEx(EWX_REBOOT|EWX_FORCE, 0);
					}
				}
		}
	return TRUE;
}

void Reboot_with_force_reboot()
{
	Force_reboot();

}

void Reboot_with_force_reboot_elevated()
{

	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-rebootforce";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void Reboot_in_safemode()
{
	if (CreateServiceSafeBootKey()) 
		{
			Set_Safemode();
			reboot();
		}

}

void Reboot_in_safemode_elevated()
{

	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = 0;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-rebootsafemode";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_HIDE;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

////////////////// ALL /////////////////////////////
////////////////////////////////////////////////////

BOOL DeleteServiceSafeBootKey()
{
	LONG lSuccess;
	char szKey[1024];
	_snprintf(szKey, 1024, "SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\%s\\%s", "Network", service_name);
	lSuccess = RegDeleteKey(HKEY_LOCAL_MACHINE, szKey);
	return lSuccess == ERROR_SUCCESS;

}

void Restore_safemode()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6)
		{
			char drivepath[150];
			char systemdrive[150];
			char stringvalue[512];
			GetEnvironmentVariable("SYSTEMDRIVE", systemdrive, 150);
			strcat (systemdrive,"/boot.ini");
			GetPrivateProfileString("boot loader","default","",drivepath,150,systemdrive);
			if (strlen(drivepath)==0) return;
			GetPrivateProfileString("operating systems",drivepath,"",stringvalue,512,systemdrive);
			if (strlen(stringvalue)==0) return;
			char* p = strrchr(stringvalue, '/');
			if (p == NULL) return;
				*p = '\0';
			WritePrivateProfileString("operating systems",drivepath,stringvalue,systemdrive);		
			SetFileAttributes(systemdrive,FILE_ATTRIBUTE_READONLY);
		}
	else
		{
#ifdef _X64
			char systemroot[150];
			GetEnvironmentVariable("SystemRoot", systemroot, 150);
			char exe_file_name[MAX_PATH];
			char parameters[MAX_PATH];
			strcpy(exe_file_name,systemroot);
			strcat(exe_file_name,"\\system32\\");
			strcat(exe_file_name,"bcdedit.exe");
			strcpy(parameters,"/deletevalue safeboot");
			SHELLEXECUTEINFO shExecInfo;
			shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
			shExecInfo.fMask = 0;
			shExecInfo.hwnd = GetForegroundWindow();
			shExecInfo.lpVerb = "runas";
			shExecInfo.lpFile = exe_file_name;
			shExecInfo.lpParameters = parameters;
			shExecInfo.lpDirectory = NULL;
			shExecInfo.nShow = SW_HIDE;
			shExecInfo.hInstApp = NULL;
			ShellExecuteEx(&shExecInfo);
#else
			typedef BOOL (WINAPI *LPFN_Wow64DisableWow64FsRedirection)(PVOID* OldValue);
			typedef BOOL (WINAPI *LPFN_Wow64RevertWow64FsRedirection)(PVOID OldValue);
			PVOID OldValue;  
			LPFN_Wow64DisableWow64FsRedirection pfnWow64DisableWowFsRedirection = (LPFN_Wow64DisableWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64DisableWow64FsRedirection");
			LPFN_Wow64RevertWow64FsRedirection pfnWow64RevertWow64FsRedirection = (LPFN_Wow64RevertWow64FsRedirection)GetProcAddress(GetModuleHandle("kernel32"),"Wow64RevertWow64FsRedirection");
			if (pfnWow64DisableWowFsRedirection && pfnWow64RevertWow64FsRedirection)  ///win32 on x64 system
			{
				if(TRUE == pfnWow64DisableWowFsRedirection(&OldValue))
					{
						char systemroot[150];
						GetEnvironmentVariable("SystemRoot", systemroot, 150);
						char exe_file_name[MAX_PATH];
						char parameters[MAX_PATH];
						strcpy(exe_file_name,systemroot);
						strcat(exe_file_name,"\\system32\\");
						strcat(exe_file_name,"bcdedit.exe");
						strcpy(parameters,"/deletevalue safeboot");
						SHELLEXECUTEINFO shExecInfo;
						shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						shExecInfo.fMask = 0;
						shExecInfo.hwnd = GetForegroundWindow();
						shExecInfo.lpVerb = "runas";
						shExecInfo.lpFile = exe_file_name;
						shExecInfo.lpParameters = parameters;
						shExecInfo.lpDirectory = NULL;
						shExecInfo.nShow = SW_HIDE;
						shExecInfo.hInstApp = NULL;
						ShellExecuteEx(&shExecInfo);
						pfnWow64RevertWow64FsRedirection(OldValue);
					}
				else
				{
					char systemroot[150];
					GetEnvironmentVariable("SystemRoot", systemroot, 150);
					char exe_file_name[MAX_PATH];
					char parameters[MAX_PATH];
					strcpy(exe_file_name,systemroot);
					strcat(exe_file_name,"\\system32\\");
					strcat(exe_file_name,"bcdedit.exe");
					strcpy(parameters,"/deletevalue safeboot");
					SHELLEXECUTEINFO shExecInfo;
					shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
					shExecInfo.fMask = 0;
					shExecInfo.hwnd = GetForegroundWindow();
					shExecInfo.lpVerb = "runas";
					shExecInfo.lpFile = exe_file_name;
					shExecInfo.lpParameters = parameters;
					shExecInfo.lpDirectory = NULL;
					shExecInfo.nShow = SW_HIDE;
					shExecInfo.hInstApp = NULL;
					ShellExecuteEx(&shExecInfo);
				}
			}
			else  //win32 on W32
			{
				char systemroot[150];
				GetEnvironmentVariable("SystemRoot", systemroot, 150);
				char exe_file_name[MAX_PATH];
				char parameters[MAX_PATH];
				strcpy(exe_file_name,systemroot);
				strcat(exe_file_name,"\\system32\\");
				strcat(exe_file_name,"bcdedit.exe");
				strcpy(parameters,"/deletevalue safeboot");
				SHELLEXECUTEINFO shExecInfo;
				shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
				shExecInfo.fMask = 0;
				shExecInfo.hwnd = GetForegroundWindow();
				shExecInfo.lpVerb = "runas";
				shExecInfo.lpFile = exe_file_name;
				shExecInfo.lpParameters = parameters;
				shExecInfo.lpDirectory = NULL;
				shExecInfo.nShow = SW_HIDE;
				shExecInfo.hInstApp = NULL;
				ShellExecuteEx(&shExecInfo);
			}
#endif
		}
}

void Restore_after_reboot()
{
	//If we are running !normal mode
	//disable boot.ini /safemode:network
	//disable safeboot service
	if (GetSystemMetrics(SM_CLEANBOOT) != 0)
	{
		Restore_safemode();
		DeleteServiceSafeBootKey();
	}
}


////////////////// >VISTA /////////////////////////////
///////////////////////////////////////////////////////
bool ISUACENabled()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return false;
	HKEY hKey;
	if (::RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", &hKey) == ERROR_SUCCESS)
		{
			DWORD value = 0;
			DWORD tt=4;
			if (::RegQueryValueExW(hKey, L"EnableLUA", NULL, NULL, (LPBYTE)&value, &tt) == ERROR_SUCCESS)
			{
			RegCloseKey(hKey);
			return (value != 0);
			}
			RegCloseKey(hKey);
		}
	return false;
}

bool IsSoftwareCadEnabled()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return true;

	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		return 0;
		}
	if (RegOpenKeyEx(hkLocal,
		"System",
		0, KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return 0;
	}

	LONG pref=0;
	ULONG type = REG_DWORD;
	ULONG prefsize = sizeof(pref);

	if (RegQueryValueEx(hkLocalKey,
			"SoftwareSASGeneration",
			NULL,
			&type,
			(LPBYTE) &pref,
			&prefsize) != ERROR_SUCCESS)
	{
			RegCloseKey(hkLocalKey);
			RegCloseKey(hkLocal);
			return false;
	}
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
	if (pref!=0) return true;
	else return false;
}

void
Enable_softwareCAD()
{							
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		return;
		}
	if (RegOpenKeyEx(hkLocal,
		"System",
		0, KEY_WRITE | KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return;
	}
	LONG pref;
	pref=1;
	RegSetValueEx(hkLocalKey, "SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE) &pref, sizeof(pref));
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
}

void delete_softwareCAD()
{
	//Beep(1000,10000);
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		return;
		}
	if (RegOpenKeyEx(hkLocal,
		"System",
		0, KEY_WRITE | KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return;
	}
	RegDeleteValue(hkLocalKey, "SoftwareSASGeneration");
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);

}

void delete_softwareCAD_elevated()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return;

	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = 0;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-delsoftwarecad";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void Enable_softwareCAD_elevated()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return;

	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = 0;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-softwarecad";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
