add_subdirectory(auth-helper)

find_package(X11 REQUIRED)
find_package(PkgConfig QUIET)
pkg_check_modules(procps libproc2)
if(procps_FOUND)
	add_definitions(-DHAVE_LIBPROC2)
else()
	pkg_check_modules(procps libprocps)
	if(procps_FOUND)
		add_definitions(-DHAVE_LIBPROCPS)
	else()
		message(FATAL_ERROR "libproc2/libprocps not found")
	endif()
endif()

pkg_check_modules(fakekey libfakekey)

include(BuildVeyonPlugin)

if(NOT fakekey_FOUND)
	set(libfakekey_SOURCES ${libfakekey_DIR}/src/libfakekey.c)
	set_source_files_properties(${libfakekey_SOURCES} PROPERTIES
		COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-pointer-sign"
		SKIP_PRECOMPILE_HEADERS TRUE)
endif()

build_veyon_plugin(linux-platform
	LinuxPlatformPlugin.cpp
	LinuxCoreFunctions.cpp
	LinuxPlatformConfigurationPage.h
	LinuxPlatformConfigurationPage.cpp
	LinuxPlatformConfigurationPage.ui
	LinuxFilesystemFunctions.cpp
	LinuxInputDeviceFunctions.cpp
	LinuxNetworkFunctions.cpp
	LinuxServerProcess.cpp
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
	LinuxServerProcess.h
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

set_source_files_properties(LinuxCoreFunctions.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

target_include_directories(linux-platform PRIVATE
	../common
	${libfakekey_DIR}
	${procps_INCLUDE_DIRS}
	)

if(WITH_QT6)
	find_package(Qt6 COMPONENTS DBus REQUIRED)
else()
	find_package(Qt5DBus REQUIRED)
endif()

target_link_libraries(linux-platform PRIVATE Qt${QT_MAJOR_VERSION}::DBus)

target_link_libraries(linux-platform PRIVATE
	${X11_LIBRARIES}
	${procps_LDFLAGS})

if(fakekey_FOUND)
	target_include_directories(linux-platform PRIVATE ${fakekey_INCLUDE_DIRS})
	target_link_libraries(linux-platform PRIVATE ${fakekey_LDFLAGS})
else()
	target_include_directories(linux-platform PRIVATE ${libfakekey_DIR})
	target_link_libraries(linux-platform PRIVATE ${X11_XTest_LIB})
endif()
