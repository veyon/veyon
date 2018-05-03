INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DULTRAVNC_VEYON_SUPPORT -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600)

BUILD_PLUGIN(windows-platform
	WindowsPlatformPlugin.cpp
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
	MOCFILES
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
	COTIRE
)

INCLUDE_DIRECTORIES(${ultravnc_DIR}/addon/ms-logon/authSSP)
INCLUDE_DIRECTORIES(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32 -lnetapi32 -luserenv -linterception)

SET_TARGET_PROPERTIES(windows-platform PROPERTIES COMPILE_FLAGS "-Wno-unknown-pragmas")
