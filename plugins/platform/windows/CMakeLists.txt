INCLUDE(BuildPlugin)

BUILD_PLUGIN(windows-platform
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
	WtsSessionManager.cpp
	SasEventListener.cpp
	XEventLog.cpp
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
	SasEventListener.h
	XEventLog.h
	windows.qrc
)

TARGET_INCLUDE_DIRECTORIES(windows-platform PRIVATE ${ultravnc_DIR}/addon/ms-logon/authSSP ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32 -lnetapi32 -luserenv -linterception)
TARGET_COMPILE_DEFINITIONS(windows-platform PRIVATE -DULTRAVNC_VEYON_SUPPORT -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600)
TARGET_COMPILE_OPTIONS(windows-platform PRIVATE "-Wno-unknown-pragmas")
