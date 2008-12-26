

// For translating all messages
// find and translate all MessageBox                          -- done
// find and translate all MsgBox                              -- done
// find and translate all sendMessage with strings            -- done
// find and copy here and delete all messages string const    -- done
// find and translate all SetWindowText                       -- done
// find and translate all SetDlgItemText                      -- done

// All VNCLOG messages are NOT translated for now


#ifdef LOCALIZATION_MESSAGES

// LOCALIZATION_MESSAGES is only declare in winvnc.cpp

char sz_ID_FAILED_INIT[64];  //    "Failed to initialise the socket system"
char sz_ID_WINVNC_USAGE[64];  //   "WinVNC Usage"
char sz_ID_ANOTHER_INST[64];  //   "Another instance of WinVNC is already running"
char sz_ID_NO_EXIST_INST[64];  // "No existing instance of WinVNC could be contacted"
char sz_ID_UNABLE_INST[64];  //    "Unable to install WinVNC service"
char sz_ID_SCM_NOT_HERE[128];  // "The SCM could not be contacted - the WinVNC service was not installed"
char sz_ID_SERV_NOT_REG[64];   // "The WinVNC service could not be registered"
char sz_ID_SERV_FAIL_ST[64];  //   "The WinVNC service failed to start"
char sz_ID_SERV_CT_MISS[128];  // "The Service Control Manager could not be contacted - the WinVNC service was not registered"
char sz_ID_SERV_OLD_REG[64];  //   "The WinVNC service is already registered"
char sz_ID_SERVHELP_UNAB[128]; // "WARNING:Unable to install the ServiceHelper hook\nGlobal user-specific registry settings will not be loaded"
char sz_ID_SERV_CT_UNREG[128]; // "The Service Control Manager could not be contacted - the WinVNC service was not unregistered"
char sz_ID_SERV_NOT_UNRG[64];  //  "The WinVNC service could not be unregistered"
char sz_ID_SERV_NCONTACT[64];  //  "The WinVNC service could not be contacted"
char sz_ID_SERVHELP_NREM[126]; // "WARNING:The ServiceHelper hook entry could not be removed from the registry"
char sz_ID_SERV_NOT_STOP[64];  //  "The WinVNC service could not be stopped"
char sz_ID_SERV_MK_UNREG[64];  // "The WinVNC service is already marked to be unregistered"
char sz_ID_SERV_NT_FOUND[64];
char sz_ID_WINVNC_ERROR[64];
char sz_ID_WINVNC_WARNIN[64];  //  "WinVNC Warning"
char sz_ID_PLUGIN_LOADIN[64];  //  "Plugin Loading"
char sz_ID_NO_PASSWD_NO_OVERRIDE_ERR[200];
char sz_ID_NO_PASSWD_NO_OVERRIDE_WARN[160];
char sz_ID_NO_PASSWD_NO_LOGON_WARN [140]; // "WARNING : This machine has no default password set.  WinVNC will present the Default Properties dialog now to allow one to be entered.";
char sz_ID_NO_OVERRIDE_ERR [200];
char sz_ID_NO_CURRENT_USER_ERR [128]; // = "The WinVNC settings for the current user are unavailable at present.";
char sz_ID_CANNOT_EDIT_DEFAULT_PREFS [128]; // = "You do not have sufficient priviliges to edit the default local WinVNC settings.";
char sz_ID_NO_PASSWORD_WARN [200]; 
char sz_ID_PLUGIN_NOT_LOAD [200]; //  "The Plugin cannot be loaded.\n\rPlease check its integrity.";
char sz_ID_MB1 [10]; //  "MB1";
char sz_ID_WVNC [10]; //  "WVNC";
char sz_ID_AUTHAD_NOT_FO [128]; //  "You selected ms-logon, but the authad.dll\nwas not found.Check you installation"
char sz_ID_WARNING [64] ; //  "WARNING";
char sz_ID_AUTH_NOT_FO [128]; //    "You selected ms-logon, but the auth.dll\nwas not found.Check you installation";
char sz_ID_DESKTOP_BITBLT_ROOT [128]; //   "vncDesktop : root device doesn't support BitBlt\n"       "WinVNC cannot be used with this graphic device driver";
char sz_ID_DESKTOP_BITBLT_MEM [128];  //   "vncDesktop : memory device doesn't support GetDIBits\n"  "WinVNC cannot be used with this graphics device driver";
char sz_ID_DESKTOP_PLANAR_NOTC [128]; //   "vncDesktop : current display is PLANAR, not CHUNKY!\n"   "WinVNC cannot be used with this graphics device driver";
char sz_ID_FAILED_CONNECT_LISTING_VIEW [64]; //  "Failed to connect to listening VNC viewer";
char sz_ID_OUTGOING_CONNECTION [64] ; //  "Outgoing Connection";
char sz_ID_UNABLE_PROC_MSLOGON [64]; //  "Unable to process MS logon";
char sz_ID_RICHED32_UNLOAD [64]; //  "Unable to load the Rich Edit (RICHED32.DLL) control!";
char sz_ID_RICHED32_DLL_LD [64]; //  "Rich Edit Dll Loading";
char sz_ID_SERV_SUCCESS_INST[200];
char sz_ID_SERV_SUCCESS_REG[200];
char sz_ID_SERV_SUCCESS_UNREG[64]; //    "The WinVNC service has been unregistered";
char sz_ID_ULTRAVNC_TEXTCHAT[128]; // "The selected client is not an Ultr@VNC Viewer !\n"	"It presumably does not support TextChat\n";
char sz_ID_ULTRAVNC_WARNING [64]; //     "Ultr@VNC Warning";
char sz_ID_NO_PLUGIN_DETECT [64]; // "No Plugin detected..."

char sz_ID_CHAT_WITH_S_ULTRAVNC [64]; //   " Chat with <%s> - Ultr@VNC"
char sz_ID_CURRENT_USER_PROP [64]; //   "WinVNC: Current User Properties"
char sz_ID_DEFAULT_SYST_PROP [64]; //  "WinVNC: Default Local System Properties"
char sz_ID_AUTOREJECT_U [64] ; //  "AutoReject:%u"
char sz_ID_AUTOACCEPT_U [64] ; //  "AutoAccept:%u"
char sz_ID_CADERROR [128];
char sz_ID_CADERRORFILE [128];
char sz_ID_CADPERMISSION [128];


int Load_Localization(HINSTANCE hInstance) 
{

   LoadString(hInstance, ID_FAILED_INIT, sz_ID_FAILED_INIT, 64 -1); 
   LoadString(hInstance, ID_WINVNC_USAGE, sz_ID_WINVNC_USAGE, 64 -1); 
   LoadString(hInstance, ID_ANOTHER_INST, sz_ID_ANOTHER_INST, 64 -1); 
   LoadString(hInstance, ID_NO_EXIST_INST, sz_ID_NO_EXIST_INST, 64 -1);
   LoadString(hInstance, ID_UNABLE_INST, sz_ID_UNABLE_INST, 64 -1); 
   LoadString(hInstance, ID_SCM_NOT_HERE, sz_ID_SCM_NOT_HERE, 128 -1); 
   LoadString(hInstance, ID_SERV_NOT_REG, sz_ID_SERV_NOT_REG, 64 -1); 
   LoadString(hInstance, ID_SERV_FAIL_ST, sz_ID_SERV_FAIL_ST, 64 -1); 
   LoadString(hInstance, ID_SERV_CT_MISS, sz_ID_SERV_CT_MISS, 128 -1); 
   LoadString(hInstance, ID_SERV_OLD_REG, sz_ID_SERV_OLD_REG, 64 -1); 
   LoadString(hInstance, ID_SERVHELP_UNAB, sz_ID_SERVHELP_UNAB, 128 -1); 
   LoadString(hInstance, ID_SERV_CT_UNREG, sz_ID_SERV_CT_UNREG, 128 -1); 
   LoadString(hInstance, ID_SERV_NOT_UNRG, sz_ID_SERV_NOT_UNRG, 64 -1);  
   LoadString(hInstance, ID_SERV_NCONTACT, sz_ID_SERV_NCONTACT, 64 -1); 
   LoadString(hInstance, ID_SERVHELP_NREM, sz_ID_SERVHELP_NREM, 126 -1); 
   LoadString(hInstance, ID_SERV_NOT_STOP, sz_ID_SERV_NOT_STOP, 64 -1);  
   LoadString(hInstance, ID_SERV_MK_UNREG, sz_ID_SERV_MK_UNREG, 64 -1); 
   LoadString(hInstance, ID_SERV_NT_FOUND, sz_ID_SERV_NT_FOUND, 64 -1);
   LoadString(hInstance, ID_WINVNC_ERROR, sz_ID_WINVNC_ERROR, 64 -1);
   LoadString(hInstance, ID_WINVNC_WARNIN, sz_ID_WINVNC_WARNIN, 64 -1); 
   LoadString(hInstance, ID_PLUGIN_LOADIN, sz_ID_PLUGIN_LOADIN, 64 -1); 
   LoadString(hInstance, ID_NO_PASSWD_NO_OVERRIDE_ERR, sz_ID_NO_PASSWD_NO_OVERRIDE_ERR, 200 -1);
   LoadString(hInstance, ID_NO_PASSWD_NO_OVERRIDE_WARN, sz_ID_NO_PASSWD_NO_OVERRIDE_WARN, 160 -1);
   LoadString(hInstance, ID_NO_PASSWD_NO_LOGON_WARN, sz_ID_NO_PASSWD_NO_LOGON_WARN , 140 -1); 
   LoadString(hInstance, ID_NO_OVERRIDE_ERR, sz_ID_NO_OVERRIDE_ERR , 200 -1);
   LoadString(hInstance, ID_NO_CURRENT_USER_ERR, sz_ID_NO_CURRENT_USER_ERR , 128 -1);
   LoadString(hInstance, ID_CANNOT_EDIT_DEFAULT_PREFS, sz_ID_CANNOT_EDIT_DEFAULT_PREFS , 128 -1); 
   LoadString(hInstance, ID_NO_PASSWORD_WARN, sz_ID_NO_PASSWORD_WARN , 200 -1); 
   LoadString(hInstance, ID_PLUGIN_NOT_LOAD, sz_ID_PLUGIN_NOT_LOAD , 200 -1);
   LoadString(hInstance, ID_MB1, sz_ID_MB1 , 10 -1); 
   LoadString(hInstance, ID_WVNC, sz_ID_WVNC , 10 -1); 
   LoadString(hInstance, ID_AUTHAD_NOT_FO, sz_ID_AUTHAD_NOT_FO , 128 -1); 
   LoadString(hInstance, ID_WARNING, sz_ID_WARNING , 64 -1) ; 
   LoadString(hInstance, ID_AUTH_NOT_FO, sz_ID_AUTH_NOT_FO , 128 -1);
   LoadString(hInstance, ID_DESKTOP_BITBLT_ROOT, sz_ID_DESKTOP_BITBLT_ROOT , 128 -1);
   LoadString(hInstance, ID_DESKTOP_BITBLT_MEM, sz_ID_DESKTOP_BITBLT_MEM , 128 -1);
   LoadString(hInstance, ID_DESKTOP_PLANAR_NOTC, sz_ID_DESKTOP_PLANAR_NOTC , 128 -1); 
   LoadString(hInstance, ID_FAILED_CONNECT_LISTING_VIEW, sz_ID_FAILED_CONNECT_LISTING_VIEW , 64 -1);
   LoadString(hInstance, ID_OUTGOING_CONNECTION, sz_ID_OUTGOING_CONNECTION , 64 -1); 
   LoadString(hInstance, ID_UNABLE_PROC_MSLOGON, sz_ID_UNABLE_PROC_MSLOGON , 64 -1);
   LoadString(hInstance, ID_RICHED32_UNLOAD, sz_ID_RICHED32_UNLOAD , 64 -1); 
   LoadString(hInstance, ID_RICHED32_DLL_LD, sz_ID_RICHED32_DLL_LD , 64 -1); 
   LoadString(hInstance, ID_SERV_SUCCESS_INST, sz_ID_SERV_SUCCESS_INST, 200 -1);
   LoadString(hInstance, ID_SERV_SUCCESS_REG, sz_ID_SERV_SUCCESS_REG, 200 -1);
   LoadString(hInstance, ID_SERV_SUCCESS_UNREG, sz_ID_SERV_SUCCESS_UNREG, 64 -1); 
   LoadString(hInstance, ID_ULTRAVNC_TEXTCHAT, sz_ID_ULTRAVNC_TEXTCHAT, 128 -1); 
   LoadString(hInstance, ID_ULTRAVNC_WARNING, sz_ID_ULTRAVNC_WARNING , 64 -1); 
   LoadString(hInstance, ID_NO_PLUGIN_DETECT, sz_ID_NO_PLUGIN_DETECT , 64 -1); 
   LoadString(hInstance, ID_CHAT_WITH_S_ULTRAVNC, sz_ID_CHAT_WITH_S_ULTRAVNC, 64 -1);
   LoadString(hInstance, ID_CURRENT_USER_PROP, sz_ID_CURRENT_USER_PROP, 64 -1); 
   LoadString(hInstance, ID_DEFAULT_SYST_PROP, sz_ID_DEFAULT_SYST_PROP, 64 -1); 
   LoadString(hInstance, ID_AUTOREJECT_U, sz_ID_AUTOREJECT_U, 64 -1);
   LoadString(hInstance, ID_AUTOACCEPT_U, sz_ID_AUTOACCEPT_U, 64 -1);
   LoadString(hInstance, ID_CADERROR, sz_ID_CADERROR , 128 -1);
   LoadString(hInstance, ID_CADERROR, sz_ID_CADERRORFILE , 128 -1);
   LoadString(hInstance, ID_CADERROR, sz_ID_CADPERMISSION , 128 -1);


  return 0;
}

#else

extern char sz_ID_FAILED_INIT[64];  //    "Failed to initialise the socket system"
extern char sz_ID_WINVNC_USAGE[64];  //   "WinVNC Usage"
extern char sz_ID_ANOTHER_INST[64];  //   "Another instance of WinVNC is already running"
extern char sz_ID_NO_EXIST_INST[64];  // "No existing instance of WinVNC could be contacted"
extern char sz_ID_UNABLE_INST[64];  //    "Unable to install WinVNC service"
extern char sz_ID_SCM_NOT_HERE[128];  // "The SCM could not be contacted - the WinVNC service was not installed"
extern char sz_ID_SERV_NOT_REG[64];   // "The WinVNC service could not be registered"
extern char sz_ID_SERV_FAIL_ST[64];  //   "The WinVNC service failed to start"
extern char sz_ID_SERV_CT_MISS[128];  // "The Service Control Manager could not be contacted - the WinVNC service was not registered"
extern char sz_ID_SERV_OLD_REG[64];  //   "The WinVNC service is already registered"
extern char sz_ID_SERVHELP_UNAB[128]; // "WARNING:Unable to install the ServiceHelper hook\nGlobal user-specific registry settings will not be loaded"
extern char sz_ID_SERV_CT_UNREG[128]; // "The Service Control Manager could not be contacted - the WinVNC service was not unregistered"
extern char sz_ID_SERV_NOT_UNRG[64];  //  "The WinVNC service could not be unregistered"
extern char sz_ID_SERV_NCONTACT[64];  //  "The WinVNC service could not be contacted"
extern char sz_ID_SERVHELP_NREM[126]; // "WARNING:The ServiceHelper hook entry could not be removed from the registry"
extern char sz_ID_SERV_NOT_STOP[64];  //  "The WinVNC service could not be stopped"
extern char sz_ID_SERV_MK_UNREG[64];  // "The WinVNC service is already marked to be unregistered"
extern char sz_ID_SERV_NT_FOUND[64];
extern char sz_ID_WINVNC_ERROR[64];
extern char sz_ID_WINVNC_WARNIN[64];  //  "WinVNC Warning"
extern char sz_ID_PLUGIN_LOADIN[64];  //  "Plugin Loading"
extern char sz_ID_NO_PASSWD_NO_OVERRIDE_ERR[200];
extern char sz_ID_NO_PASSWD_NO_OVERRIDE_WARN[160];
extern char sz_ID_NO_PASSWD_NO_LOGON_WARN [140]; // "WARNING : This machine has no default password set.  WinVNC will present the Default Properties dialog now to allow one to be entered.";
extern char sz_ID_NO_OVERRIDE_ERR [200];
extern char sz_ID_NO_CURRENT_USER_ERR [128]; // = "The WinVNC settings for the current user are unavailable at present.";
extern char sz_ID_CANNOT_EDIT_DEFAULT_PREFS [128]; // = "You do not have sufficient priviliges to edit the default local WinVNC settings.";
extern char sz_ID_NO_PASSWORD_WARN [200]; 
extern char sz_ID_PLUGIN_NOT_LOAD [200]; //  "The Plugin cannot be loaded.\n\rPlease check its integrity.";
extern char sz_ID_MB1 [10]; //  "MB1";
extern char sz_ID_WVNC [10]; //  "WVNC";
extern char sz_ID_AUTHAD_NOT_FO [128]; //  "You selected ms-logon, but the authad.dll\nwas not found.Check you installation"
extern char sz_ID_WARNING [64] ; //  "WARNING";
extern char sz_ID_AUTH_NOT_FO [128]; //    "You selected ms-logon, but the auth.dll\nwas not found.Check you installation";
extern char sz_ID_DESKTOP_BITBLT_ROOT [128]; //   "vncDesktop : root device doesn't support BitBlt\n"       "WinVNC cannot be used with this graphic device driver";
extern char sz_ID_DESKTOP_BITBLT_MEM [128];  //   "vncDesktop : memory device doesn't support GetDIBits\n"  "WinVNC cannot be used with this graphics device driver";
extern char sz_ID_DESKTOP_PLANAR_NOTC [128]; //   "vncDesktop : current display is PLANAR, not CHUNKY!\n"   "WinVNC cannot be used with this graphics device driver";
extern char sz_ID_FAILED_CONNECT_LISTING_VIEW [64]; //  "Failed to connect to listening VNC viewer";
extern char sz_ID_OUTGOING_CONNECTION [64] ; //  "Outgoing Connection";
extern char sz_ID_UNABLE_PROC_MSLOGON [64]; //  "Unable to process MS logon";
extern char sz_ID_RICHED32_UNLOAD [64]; //  "Unable to load the Rich Edit (RICHED32.DLL) control!";
extern char sz_ID_RICHED32_DLL_LD [64]; //  "Rich Edit Dll Loading";
extern char sz_ID_SERV_SUCCESS_INST[200];
extern char sz_ID_SERV_SUCCESS_REG[200];
extern char sz_ID_SERV_SUCCESS_UNREG[64]; //    "The WinVNC service has been unregistered";
extern char sz_ID_ULTRAVNC_TEXTCHAT[128]; // "The selected client is not an Ultr@VNC Viewer !\n"	"It presumably does not support TextChat\n";
extern char sz_ID_ULTRAVNC_WARNING [64]; //     "Ultr@VNC Warning";

extern char sz_ID_NO_PLUGIN_DETECT [64]; // "No Plugin detected..."

extern char sz_ID_CHAT_WITH_S_ULTRAVNC [64]; //   " Chat with <%s> - Ultr@VNC"
extern char sz_ID_CURRENT_USER_PROP [64]; //   "WinVNC: Current User Properties"
extern char sz_ID_DEFAULT_SYST_PROP [64]; //  "WinVNC: Default Local System Properties"
extern char sz_ID_AUTOREJECT_U [64] ; //  "AutoReject:%u"
extern char sz_ID_AUTOACCEPT_U [64] ; //  "AutoReject:%u"
extern char sz_ID_CADERROR [128];
extern char sz_ID_CADERRORFILE [128];
extern char sz_ID_CADPERMISSION [128];

#endif
