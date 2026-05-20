add_subdirectory(auth-helper)

include(BuildVeyonPlugin)

if(WITH_QT6)
	find_package(Qt6 COMPONENTS DBus REQUIRED)
else()
	find_package(Qt5DBus REQUIRED)
endif()

find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)

build_veyon_plugin(linux-platform
	NAME LinuxPlatform
	SOURCES
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
)

set_source_files_properties(LinuxCoreFunctions.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

target_include_directories(linux-platform PRIVATE ../common)

target_link_libraries(linux-platform PRIVATE
	Qt${QT_MAJOR_VERSION}::DBus
	${X11_LIBRARIES}
)

pkg_check_modules(procps libproc2)

if(procps_FOUND)
	target_compile_definitions(linux-platform PRIVATE -DHAVE_LIBPROC2)
	include(CheckCXXSourceCompiles)
	check_cxx_source_compiles("#include <libproc2/pids.h>
		int main()
		{
		pids_stack* stack = nullptr;
		(void) PIDS_VAL(0, s_int, stack);
		}
		" LIBPROC2_PIDS_VAL_NEW_API)
	if (NOT LIBPROC2_PIDS_VAL_NEW_API)
		target_compile_definitions(linux-platform PRIVATE -DLIBPROC2_PIDS_VAL_OLD_API)
	endif()
else()
	pkg_check_modules(procps libprocps)
	if(procps_FOUND)
		target_compile_definitions(linux-platform PRIVATE -DHAVE_LIBPROCPS)
	else()
		message(FATAL_ERROR "libproc2/libprocps not found")
	endif()
endif()

target_include_directories(linux-platform PRIVATE ${procps_INCLUDE_DIRS})
target_link_libraries(linux-platform PRIVATE ${procps_LDFLAGS})

pkg_check_modules(fakekey libfakekey)

if(fakekey_FOUND)
	target_include_directories(linux-platform PRIVATE ${fakekey_INCLUDE_DIRS})
	target_link_libraries(linux-platform PRIVATE ${fakekey_LDFLAGS})
else()
	target_sources(linux-platform PRIVATE ${libfakekey_DIR}/src/libfakekey.c)
	set_source_files_properties(${libfakekey_DIR}/src/libfakekey.c PROPERTIES
		COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-pointer-sign"
		SKIP_PRECOMPILE_HEADERS TRUE)
	target_include_directories(linux-platform PRIVATE ${libfakekey_DIR})
	target_link_libraries(linux-platform PRIVATE ${X11_XTest_LIB})
endif()
