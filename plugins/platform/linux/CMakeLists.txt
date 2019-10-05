ADD_SUBDIRECTORY(auth-helper)

FIND_PACKAGE(X11 REQUIRED)
FIND_PACKAGE(Qt5DBus REQUIRED)
FIND_PACKAGE(PkgConfig QUIET)
pkg_check_modules(procps REQUIRED libprocps)

INCLUDE(BuildPlugin)

BUILD_PLUGIN(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxPlatformConfigurationPage.h
	LinuxPlatformConfigurationPage.cpp
	LinuxPlatformConfigurationPage.ui
	LinuxFilesystemFunctions.cpp
	LinuxInputDeviceFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxServiceCore.cpp
	LinuxServiceFunctions.cpp
	LinuxUserFunctions.cpp
	LinuxPlatformPlugin.h
	LinuxPlatformConfiguration.h
	LinuxCoreFunctions.h
	LinuxDesktopIntegration.h
	LinuxFilesystemFunctions.h
	LinuxInputDeviceFunctions.h
	LinuxKeyboardShortcutTrapper.h
	LinuxNetworkFunctions.h
	LinuxServiceCore.h
	LinuxServiceFunctions.h
	LinuxUserFunctions.h
	linux.qrc
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
)

TARGET_INCLUDE_DIRECTORIES(linux-platform PRIVATE
	../common
	${procps_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(linux-platform
	${X11_LIBRARIES}
	Qt5::DBus
	${procps_LDFLAGS}
	)
