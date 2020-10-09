ADD_SUBDIRECTORY(auth-helper)

FIND_PACKAGE(X11 REQUIRED)
FIND_PACKAGE(Qt5DBus REQUIRED)
FIND_PACKAGE(PkgConfig QUIET)
pkg_check_modules(procps REQUIRED libprocps)
pkg_check_modules(fakekey libfakekey)

INCLUDE(BuildVeyonPlugin)

IF(NOT fakekey_FOUND)
SET(libfakekey_SOURCES ${libfakekey_DIR}/src/libfakekey.c)
SET_SOURCE_FILES_PROPERTIES(${libfakekey_SOURCES} PROPERTIES
	COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-pointer-sign"
	COTIRE_EXCLUDED TRUE)
ENDIF()

build_veyon_plugin(linux-platform
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
	LinuxSessionFunctions.cpp
	LinuxUserFunctions.cpp
	LinuxPlatformPlugin.h
	LinuxPlatformConfiguration.h
	LinuxCoreFunctions.h
	LinuxDesktopIntegration.h
	LinuxFilesystemFunctions.h
	LinuxInputDeviceFunctions.h
	LinuxKeyboardInput.h
	LinuxKeyboardInput.cpp
	LinuxKeyboardShortcutTrapper.h
	LinuxNetworkFunctions.h
	LinuxServiceCore.h
	LinuxServiceFunctions.h
	LinuxSessionFunctions.h
	LinuxUserFunctions.h
	linux.qrc
	../common/LogonHelper.h
	../common/LogonHelper.cpp
	../common/PersistentLogonCredentials.h
	../common/PersistentLogonCredentials.cpp
	../common/PlatformSessionManager.h
	../common/PlatformSessionManager.cpp
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
	${libfakekey_SOURCES}
)

TARGET_INCLUDE_DIRECTORIES(linux-platform PRIVATE
	../common
	${libfakekey_DIR}
	${procps_INCLUDE_DIRS}
)

TARGET_LINK_LIBRARIES(linux-platform
	${X11_LIBRARIES}
	Qt5::DBus
	${procps_LDFLAGS}
	)

IF(fakekey_FOUND)
TARGET_INCLUDE_DIRECTORIES(linux-platform PRIVATE ${fakekey_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(linux-platform ${fakekey_LDFLAGS})
ELSE()
TARGET_INCLUDE_DIRECTORIES(linux-platform PRIVATE ${libfakekey_DIR})
TARGET_LINK_LIBRARIES(linux-platform ${X11_XTest_LIB})
ENDIF()
