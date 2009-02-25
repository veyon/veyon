#include "stdhdrs.h"
#include <windows.h>
#include <Wtsapi32.h>
#include "common/win32_helpers.h"

static void WINAPI service_main(DWORD, LPTSTR *);
static void WINAPI control_handler(DWORD controlCode);
static DWORD WINAPI control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
static int pad();

static SERVICE_STATUS serviceStatus;
static SERVICE_STATUS_HANDLE serviceStatusHandle=0;
HANDLE stopServiceEvent=0;
extern HANDLE hEvent;
static char service_path[MAX_PATH];
void monitor_sessions();
char service_name[256]="uvnc_service";
char *app_name = "UltraVNC";
void disconnect_remote_sessions();
char cmdtext[256];
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

monitor_sessions();

        /* service was stopped */
        serviceStatus.dwCurrentState=SERVICE_STOP_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

        /* do cleanup here */
        CloseHandle(stopServiceEvent);
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
                disconnect_remote_sessions();
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
	strcpy(cmdtext,cmd);
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

int install_service(void) {
    SC_HANDLE scm, service;
	pad();

    scm=OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);
    if(!scm) {
        MessageBox(NULL, "Failed to open service control manager",
            app_name, MB_ICONERROR);
        return 1;
    }
    //"Provides secure remote desktop sharing"
    service=CreateService(scm,service_name, service_name, SERVICE_ALL_ACCESS,
                          SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                          SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, service_path,
        NULL, NULL, NULL, NULL, NULL);
    if(!service) {
		DWORD myerror=GetLastError();
		if (myerror==ERROR_ACCESS_DENIED)
		{
			MessageBox(NULL, "Failed: Permission denied",
            app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror==ERROR_SERVICE_EXISTS)
		{
			//MessageBox(NULL, "Failed: Already exist",
            //"UltraVnc", MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}

        MessageBox(NULL, "Failed to create a new service",
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
        MessageBox(NULL, "Failed to open service control manager",
            app_name, MB_ICONERROR);
        return 1;
    }

	service=OpenService(scm, service_name,
        SERVICE_QUERY_STATUS | DELETE);
    if(!service) {
		DWORD myerror=GetLastError();
		if (myerror==ERROR_ACCESS_DENIED)
		{
			MessageBox(NULL, "Failed: Permission denied",
            app_name, MB_ICONERROR);
			CloseServiceHandle(scm);
			return 1;
		}
		if (myerror==ERROR_SERVICE_DOES_NOT_EXIST)
		{
#if 0
			MessageBox(NULL, "Failed: Service is not installed",
            app_name, MB_ICONERROR);
#endif
			CloseServiceHandle(scm);
			return 1;
		}

        MessageBox(NULL, "Failed to open the service",
            app_name, MB_ICONERROR);
        CloseServiceHandle(scm);
        return 1;
    }
    if(!QueryServiceStatus(service, &serviceStatus)) {
        MessageBox(NULL, "Failed to query service status",
            app_name, MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
        return 1;
    }
    if(serviceStatus.dwCurrentState!=SERVICE_STOPPED) {
        //MessageBox(NULL, "The service is still running, disable it first",
        //    "UltraVnc", MB_ICONERROR);
        CloseServiceHandle(service);
        CloseServiceHandle(scm);
		Sleep(2500);uninstall_service();
        return 1;
    }
    if(!DeleteService(service)) {
        MessageBox(NULL, "Failed to delete the service",
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
