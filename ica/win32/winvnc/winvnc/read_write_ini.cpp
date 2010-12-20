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

#include "inifile.h"

bool do_copy (IniFile& myIniFile_In, IniFile& myIniFile_Out)
{

TCHAR *group1=new char[150];
TCHAR *group2=new char[150];
TCHAR *group3=new char[150];
BOOL BUseRegistry;
LONG MSLogonRequired;
LONG NewMSLogon;
LONG locdom1;
LONG locdom2;
LONG locdom3;
LONG DebugMode;
LONG Avilog;
LONG DebugLevel;
LONG DisableTrayIcon;
LONG LoopbackOnly;
LONG UseDSMPlugin;
LONG AllowLoopback;
LONG AuthRequired;
LONG ConnectPriority;

char DSMPlugin[128];
char *authhosts=new char[150];

LONG AllowShutdown;
LONG AllowProperties;
LONG AllowEditClients;

LONG FileTransferEnabled;
LONG FTUserImpersonation;
LONG BlankMonitorEnabled ;
LONG BlankInputsOnly; //PGM
LONG DefaultScale ;
LONG CaptureAlphaBlending ;
LONG BlackAlphaBlending ;

LONG SocketConnect;
LONG HTTPConnect;
LONG XDMCPConnect;
LONG AutoPortSelect;
LONG PortNumber;
LONG HttpPortNumber;
LONG IdleTimeout;

LONG RemoveWallpaper;
LONG RemoveAero;

LONG QuerySetting;
LONG QueryTimeout;
LONG QueryAccept;
LONG QueryIfNoLogon;

LONG EnableRemoteInputs;
LONG LockSettings;
LONG DisableLocalInputs;
LONG EnableJapInput;
LONG kickrdp;
LONG clearconsole;

#define MAXPWLEN 8
char passwd[MAXPWLEN];

LONG TurboMode;
LONG PollUnderCursor;
LONG PollForeground;
LONG PollFullScreen;
LONG PollConsoleOnly;
LONG PollOnEventOnly;
LONG MaxCpu;
LONG Driver;
LONG Hook;
LONG Virtual;
LONG SingleWindow=0;
char SingleWindowName[32];
LONG FTTimeout = 30;
char path[512];

LONG Primary=1;
LONG Secondary=0;
//Beep(100,20000);
BUseRegistry = myIniFile_In.ReadInt("admin", "UseRegistry", 0);
if (!myIniFile_Out.WriteInt("admin", "UseRegistry", BUseRegistry))
{
		//error
		char temp[10];
		DWORD error=GetLastError();
		MessageBox(NULL,myIniFile_Out.myInifile,_itoa(error,temp,10),MB_ICONERROR);
		return false;
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
BlankInputsOnly = myIniFile_In.ReadInt("admin", "BlankInputsOnly", false); //PGM
DefaultScale = myIniFile_In.ReadInt("admin", "DefaultScale", 1);
CaptureAlphaBlending = myIniFile_In.ReadInt("admin", "CaptureAlphaBlending", false); // sf@2005
BlackAlphaBlending = myIniFile_In.ReadInt("admin", "BlackAlphaBlending", false); // sf@2005
FTTimeout = myIniFile_In.ReadInt("admin", "FileTransferTimeout", 30);

Primary = myIniFile_In.ReadInt("admin", "primary", true);
Secondary = myIniFile_In.ReadInt("admin", "secondary", false);


myIniFile_Out.WriteInt("admin", "FileTransferEnabled", FileTransferEnabled);
myIniFile_Out.WriteInt("admin", "FTUserImpersonation", FTUserImpersonation);
myIniFile_Out.WriteInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
myIniFile_Out.WriteInt("admin", "BlankInputsOnly", BlankInputsOnly); //PGM
myIniFile_Out.WriteInt("admin", "DefaultScale", DefaultScale);
myIniFile_Out.WriteInt("admin", "CaptureAlphaBlending", CaptureAlphaBlending);
myIniFile_Out.WriteInt("admin", "BlackAlphaBlending", BlackAlphaBlending);
myIniFile_Out.WriteInt("admin", "FileTransferTimeout", 30);

myIniFile_Out.WriteInt("admin", "primary", Primary);
myIniFile_Out.WriteInt("admin", "secondary", Secondary);

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
memset(passwd, '\0', MAXPWLEN); //PGM 
myIniFile_In.ReadPassword2(passwd,MAXPWLEN); //PGM
myIniFile_Out.WritePassword2(passwd); //PGM

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
return true;
}

bool Copy_to_Temp(char *tempfile)
{
IniFile myIniFile_In;
IniFile myIniFile_Out;
myIniFile_In.IniFileSetSecure();
myIniFile_Out.IniFileSetTemp(tempfile);

return do_copy(myIniFile_In, myIniFile_Out);

}


bool Copy_to_Secure_from_temp(char *tempfile)
{
IniFile myIniFile_In;
IniFile myIniFile_Out;
myIniFile_Out.IniFileSetSecure();
myIniFile_In.IniFileSetTemp(tempfile);

return do_copy(myIniFile_In, myIniFile_Out);
}