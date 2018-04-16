ADD_SUBDIRECTORY(auth-helper)

FIND_PACKAGE(X11 REQUIRED)
FIND_PACKAGE(Qt5DBus REQUIRED)
FIND_PACKAGE(PkgConfig QUIET)
pkg_check_modules(procps REQUIRED libprocps)

INCLUDE(BuildPlugin)

INCLUDE_DIRECTORIES(${procps_INCLUDE_DIRS})

BUILD_PLUGIN(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxFilesystemFunctions.cpp
	LinuxInputDeviceFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxServiceCore.cpp
	LinuxServiceFunctions.cpp
	LinuxUserFunctions.cpp
	MOCFILES
	LinuxPlatformPlugin.h
	LinuxCoreFunctions.h
	LinuxDesktopIntegration.h
	LinuxFilesystemFunctions.h
	LinuxInputDeviceFunctions.h
	LinuxKeyboardShortcutTrapper.h
	LinuxNetworkFunctions.h
	LinuxServiceCore.h
	LinuxServiceFunctions.h
	LinuxUserFunctions.h
	COTIRE
)

TARGET_LINK_LIBRARIES(linux-platform
	${X11_LIBRARIES}
	Qt5::DBus
	${procps_LDFLAGS}
	)
