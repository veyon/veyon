add_subdirectory(auth-helper)

find_package(X11 REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(PkgConfig QUIET)
pkg_check_modules(procps REQUIRED libprocps)
pkg_check_modules(fakekey libfakekey)

include(BuildPlugin)

if(NOT fakekey_FOUND)
set(libfakekey_SOURCES ${libfakekey_DIR}/src/libfakekey.c)
set_source_files_properties(${libfakekey_SOURCES} PROPERTIES
	COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-pointer-sign"
	COTIRE_EXCLUDED TRUE)
endif()

build_plugin(linux-platform
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
	LinuxSmartcardFunctions.cpp
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
	LinuxUserFunctions.h
	linux.qrc
	../common/LogonHelper.h
	../common/LogonHelper.cpp
	../common/PersistentLogonCredentials.h
	../common/PersistentLogonCredentials.cpp
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
	${libfakekey_SOURCES}
)

target_include_directories(linux-platform PRIVATE
	../common
	${libfakekey_DIR}
	${procps_INCLUDE_DIRS}
)

target_link_libraries(linux-platform
	${X11_LIBRARIES}
	Qt5::DBus
	${procps_LDFLAGS}
	)

if(fakekey_FOUND)
target_include_directories(linux-platform PRIVATE ${fakekey_INCLUDE_DIRS})
target_link_libraries(linux-platform ${fakekey_LDFLAGS})
else()
target_include_directories(linux-platform PRIVATE ${libfakekey_DIR})
target_link_libraries(linux-platform ${X11_XTest_LIB})
endif()
