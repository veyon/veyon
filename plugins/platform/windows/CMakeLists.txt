include(BuildPlugin)

add_definitions(-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600)

build_plugin(windows-platform
	WindowsPlatformPlugin.cpp
	WindowsPlatformConfigurationPage.h
	WindowsPlatformConfigurationPage.cpp
	WindowsPlatformConfigurationPage.ui
	WindowsCoreFunctions.cpp
	WindowsFilesystemFunctions.cpp
	WindowsInputDeviceFunctions.cpp
	WindowsKeyboardShortcutTrapper.cpp
	WindowsNetworkFunctions.cpp
	WindowsServiceControl.cpp
	WindowsServiceCore.cpp
	WindowsServiceFunctions.cpp
	WindowsUserFunctions.cpp
	WindowsSmartcardFunctions.cpp
	WtsSessionManager.cpp
	DesktopInputController.cpp
	SasEventListener.cpp
	XEventLog.cpp
	${ultravnc_DIR}/winvnc/winvnc/vnckeymap.cpp
	${ultravnc_DIR}/addon/ms-logon/authSSP/authSSP.cpp
	${ultravnc_DIR}/addon/ms-logon/authSSP/GenClientServerContext.cpp
	WindowsPlatformConfiguration.h
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsFilesystemFunctions.h
	WindowsInputDeviceFunctions.h
	WindowsKeyboardShortcutTrapper.h
	WindowsNetworkFunctions.h
	WindowsServiceControl.h
	WindowsServiceCore.h
	WindowsServiceFunctions.h
	WindowsUserFunctions.h
	WtsSessionManager.h
	DesktopInputController.h
	SasEventListener.h
	../common/LogonHelper.h
	../common/LogonHelper.cpp
	../common/PersistentLogonCredentials.h
	../common/PersistentLogonCredentials.cpp
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
	XEventLog.h
	windows.qrc
)

target_include_directories(windows-platform PRIVATE
	../common
	${ultravnc_DIR}/addon/ms-logon/authSSP
	${ultravnc_DIR}/winvnc/winvnc
	${ultravnc_DIR}
	${Qt5Gui_PRIVATE_INCLUDE_DIRS}
)
target_link_libraries(windows-platform -lwtsapi32 -lnetapi32 -luserenv -linterception -lcrypt32)
target_compile_definitions(windows-platform PRIVATE ULTRAVNC_VEYON_SUPPORT)
target_compile_options(windows-platform PRIVATE "-Wno-unknown-pragmas")
