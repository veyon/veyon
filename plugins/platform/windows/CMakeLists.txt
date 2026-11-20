include(BuildVeyonPlugin)

set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)

# keyboard input for the user logon (keysym mapping) and SSPI-based user authentication
# are based on bundled UltraVNC sources
if(WITH_ULTRAVNC)
	set(windows_platform_input_SOURCES
		DesktopInputController.cpp
		DesktopInputController.h
		)
	set(windows_platform_ultravnc_SOURCES
		${ultravnc_DIR}/winvnc/winvnc/vnckeymap.cpp
		${ultravnc_DIR}/addon/ms-logon/authSSP/authSSP.cpp
		${ultravnc_DIR}/addon/ms-logon/authSSP/GenClientServerContext.cpp
		)
endif()

build_veyon_plugin(windows-platform
	NAME WindowsPlatform
	SOURCES
	WindowsPlatformPlugin.cpp
	WindowsPlatformConfigurationPage.h
	WindowsPlatformConfigurationPage.cpp
	WindowsPlatformConfigurationPage.ui
	WindowsCoreFunctions.cpp
	WindowsDeviceFunctions.cpp
	WindowsFilesystemFunctions.cpp
	WindowsLogFileDirectory.cpp
	WindowsInputDeviceFunctions.cpp
	WindowsKeyboardShortcutTrapper.cpp
	WindowsNetworkFunctions.cpp
	WindowsSecretFunctions.cpp
	WindowsServerProcess.cpp
	WindowsServiceControl.cpp
	WindowsServiceCore.cpp
	WindowsServiceFunctions.cpp
	WindowsSessionFunctions.cpp
	WindowsUserFunctions.cpp
	WtsSessionManager.cpp
	${windows_platform_input_SOURCES}
	SasEventListener.cpp
	XEventLog.cpp
	${windows_platform_ultravnc_SOURCES}

	WindowsPlatformConfiguration.h
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsDeviceFunctions.h
	WindowsFilesystemFunctions.h
	WindowsLogFileDirectory.h
	WindowsInputDeviceFunctions.h
	WindowsKeyboardShortcutTrapper.h
	WindowsNetworkFunctions.h
	WindowsSecretFunctions.h
	WindowsServerProcess.h
	WindowsServiceControl.h
	WindowsServiceCore.h
	WindowsServiceFunctions.h
	WindowsSessionFunctions.h
	WindowsSmartObjects.h
	WindowsUserFunctions.h
	WtsSessionManager.h
	SasEventListener.h
	../common/AuthenticationLimiter.h
	../common/AuthenticationLimiter.cpp
	../common/IpcAuthenticationRequest.h
	../common/IpcAuthenticationRequest.cpp
	../common/LogonHelper.h
	../common/LogonHelper.cpp
	../common/PersistentLogonCredentials.h
	../common/PersistentLogonCredentials.cpp
	../common/PlatformSessionManager.h
	../common/PlatformSessionManager.cpp
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
	../common/ServiceIpcCommandHandler.h
	../common/ServiceIpcCommandHandler.cpp
	XEventLog.h
	windows.qrc
	)

target_include_directories(windows-platform PRIVATE ../common)

if(WITH_ULTRAVNC)
	target_include_directories(windows-platform PRIVATE
		${ultravnc_DIR}/addon/ms-logon/authSSP
		${ultravnc_DIR}/winvnc/winvnc
		${ultravnc_DIR}
		)
	target_compile_definitions(windows-platform PRIVATE ULTRAVNC_VEYON_SUPPORT VEYON_WITH_ULTRAVNC)
else()
	message(STATUS "windows-platform: building without UltraVNC - SSPI-based user authentication and user logon via keyboard input are not available")
endif()

target_link_libraries(windows-platform PRIVATE -lws2_32 -lwtsapi32 -lnetapi32 -luserenv -limm32 -lntdll -linterception -liphlpapi -lsetupapi -lsecur32 -lcrypt32)
target_link_libraries(windows-platform PRIVATE Qt6::GuiPrivate)
target_compile_options(windows-platform PRIVATE "-Wno-unknown-pragmas;-flarge-source-files;-Wno-misleading-indentation")
set_source_files_properties(WindowsNetworkFunctions.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE SKIP_PRECOMPILE_HEADERS TRUE)
