CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

PROJECT(veyon)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
    SET(VEYON_DEBUG TRUE)
ELSEIF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE relwithdebinfo)
ENDIF()

IF(VEYON_DEBUG)
    ADD_DEFINITIONS(-DVEYON_DEBUG)
ENDIF()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0009 NEW)
	CMAKE_POLICY(SET CMP0020 NEW)
	CMAKE_POLICY(SET CMP0063 NEW)
	if(${CMAKE_VERSION} VERSION_GREATER "3.12.0")
		cmake_policy(SET CMP0075 NEW)
	endif()
	if(${CMAKE_VERSION} VERSION_GREATER "3.14.0")
		cmake_policy(SET CMP0083 NEW)
	endif()
ENDIF()

INCLUDE(AddFileDependencies)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(GNUInstallDirs)
INCLUDE(CotireVeyon)
INCLUDE(ConfigureFiles)
include(SetDefaultTargetProperties)

FIND_PACKAGE(Git)

IF(GIT_FOUND)
    EXECUTE_PROCESS(COMMAND "${GIT_EXECUTABLE}" describe --tags
		            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
					OUTPUT_STRIP_TRAILING_WHITESPACE
					OUTPUT_VARIABLE VERSION_STRING)
	STRING(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION_STRING}")
	STRING(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION_STRING}")
	STRING(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION_STRING}")

	# determine build number to use in NSIS installer and resource files
	EXECUTE_PROCESS(COMMAND "${GIT_EXECUTABLE}" describe --tags
		            COMMAND cut -d "-" -f2
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
					OUTPUT_STRIP_TRAILING_WHITESPACE
					OUTPUT_VARIABLE VERSION_BUILD)
	IF(NOT VERSION_BUILD GREATER 0)
	    SET(VERSION_BUILD 0)
	ENDIF()

	# Get list of all committers from git history, ordered by number of commits.
	# The CONTRIBUTORS file is used by AboutDialog. This information can be provided
	# with -DCONTRIBUTORS=/path/to/CONTRIBUTORS instead. For instance, to generate
	# this file for version 3.0.2, the command is:
	#   git shortlog -sne v3.0.2 | cut -c8-
	SET(CONTRIBUTORS "${CMAKE_BINARY_DIR}/CONTRIBUTORS")
	IF(NOT EXISTS "${CONTRIBUTORS}")
		EXECUTE_PROCESS(COMMAND "${GIT_EXECUTABLE}" shortlog -s d160d147165271516589c304cb1b8f5e48f8527d..HEAD
			            COMMAND cut -c8-
						COMMAND sort -f
						OUTPUT_FILE "${CONTRIBUTORS}"
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
						TIMEOUT 10)
	ENDIF()

ENDIF()

# can't retrieve version information as not building from Git repository?
IF(NOT VERSION_STRING)
    SET(VERSION_MAJOR 4)
	SET(VERSION_MINOR 3)
	SET(VERSION_PATCH 5)
	SET(VERSION_BUILD 0)
	SET(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
ELSE()
    # remove leading character from tag name
	STRING(REPLACE "v" "" VERSION_STRING "${VERSION_STRING}")
ENDIF()

# set up compiler version variable
STRING(REGEX REPLACE "\\.[0-9]$" "" COMPILER_VERSION_MAJOR_MINOR ${CMAKE_CXX_COMPILER_VERSION})


# set up basic platform variables
IF(WIN32)
    SET(VEYON_BUILD_WIN32 1)
ENDIF()
IF(APPLE)
	SET(VEYON_BUILD_APPLE 1)
ENDIF()
IF(UNIX AND NOT ANDROID)
	SET(VEYON_BUILD_LINUX 1)
ENDIF()
IF(ANDROID)
	SET(VEYON_BUILD_ANDROID 1)
ENDIF()

IF(WIN64)
    SET(VEYON_BUILD_WIN64 TRUE)
ENDIF(WIN64)

# set up library and plugin path variables
IF(VEYON_BUILD_ANDROID)
	SET(CMAKE_INSTALL_PREFIX "/")
	SET(VEYON_LIB_DIR "libs/${ANDROID_ABI}")
	SET(VEYON_INSTALL_PLUGIN_DIR "${VEYON_LIB_DIR}/veyon")
	SET(VEYON_INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/veyon")
	SET(VEYON_PLUGIN_DIR "")
	SET(VEYON_TRANSLATIONS_DIR "/translations")
ELSE()
	IF(CMAKE_INSTALL_LIBDIR)
		SET(VEYON_LIB_DIR "${CMAKE_INSTALL_LIBDIR}/veyon" CACHE INTERNAL "Veyon library directory")
	ELSE()
		SET(VEYON_LIB_DIR lib/veyon CACHE INTERNAL "Veyon library directory")
	ENDIF()

	SET(VEYON_INSTALL_PLUGIN_DIR "${VEYON_LIB_DIR}")
	SET(VEYON_INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/veyon")

	IF(WIN32)
		SET(VEYON_PLUGIN_DIR "plugins")
		SET(VEYON_TRANSLATIONS_DIR "translations")
	ELSE()
		SET(VEYON_PLUGIN_DIR "../${VEYON_LIB_DIR}")
		SET(VEYON_TRANSLATIONS_DIR "../share/veyon/translations")
	ENDIF()
ENDIF()


SET(VEYON_CORE_INCLUDE_DIR core/include)

# find required Qt5 modules
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5LinguistTools REQUIRED)
IF(VEYON_DEBUG)
FIND_PACKAGE(Qt5Test REQUIRED)
SET(VEYON_DEBUG_LIBRARIES Qt5::Test)
ENDIF()

# find required libraries
find_package(QCA REQUIRED)
find_package(OpenSSL REQUIRED)

# find Linux-specific packages
IF(VEYON_BUILD_LINUX)
	INCLUDE(XdgInstall)
ENDIF()

find_package(LibVNCClient 0.9.13)

if(LibVNCClient_FOUND)
	include(CheckCSourceCompiles)
	set(CMAKE_REQUIRED_LIBRARIES LibVNC::LibVNCClient)
	check_c_source_compiles("
#include <rfb/rfbclient.h>

int main()
{
	rfbClient* client = rfbGetClient( 8, 3, 4 );
	client->connectTimeout = 1;
	client->readTimeout = 1;
	return 0;
}
"
	LIBVNCCLIENT_SUPPORTS_TIMEOUTS)
	if(NOT LIBVNCCLIENT_SUPPORTS_TIMEOUTS)
		message(FATAL_ERROR "Outdated development version of LibVNCClient found")
	endif()
	unset(CMAKE_REQUIRED_LIBRARIES)
else()
	message(WARNING "Performing internal build of LibVNCClient which requires additional development packages")
	find_package(ZLIB REQUIRED)
	find_package(PNG REQUIRED)
	find_package(JPEG REQUIRED)
	find_package(LZO REQUIRED)
	set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
	find_package(Threads REQUIRED)
endif()

IF(SANITIZE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fsanitize=undefined")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fsanitize=undefined")
ENDIF()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-exceptions -std=c++11 -fstack-protector-strong ${CXXFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fstack-protector-strong ${CFLAGS}")

IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
MESSAGE(WARNING "Not using -fvisibility=hidden as it is broken with GCC < 6.0")
ELSE()
SET(CMAKE_C_VISIBILITY_PRESET hidden)
SET(CMAKE_CXX_VISIBILITY_PRESET hidden)
SET(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
IF(LTO)
SET(LTO_FLAGS "-flto=4")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LTO_FLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LTO_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LTO_FLAGS}")
SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LTO_FLAGS}")
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LTO_FLAGS}")
ENDIF()
ENDIF()

add_definitions(-DQT_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x050e00 -D_FORTIFY_SOURCE=2 -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_NO_KEYWORDS)

FILE(GLOB_RECURSE IN_FILES RELATIVE ${CMAKE_SOURCE_DIR} "*config.h.in" "*.rc.in" "*.desktop.in" "*.policy.in" "*.service.in" "*.manifest.in" "*.nsi.in")
CONFIGURE_FILES(${IN_FILES})

SET(CMAKE_AUTOMOC TRUE)
SET(CMAKE_AUTOUIC TRUE)
SET(CMAKE_AUTORCC TRUE)

SET(3rdparty_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
SET(ultravnc_DIR ${3rdparty_DIR}/ultravnc)
SET(libvncserver_DIR ${3rdparty_DIR}/libvncserver)
SET(x11vnc_DIR ${3rdparty_DIR}/x11vnc)
SET(libfakekey_DIR ${3rdparty_DIR}/libfakekey)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${VEYON_LIB_DIR}")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# make sub-directories
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(server)
ADD_SUBDIRECTORY(service)
ADD_SUBDIRECTORY(master)
ADD_SUBDIRECTORY(configurator)
ADD_SUBDIRECTORY(cli)
ADD_SUBDIRECTORY(worker)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(translations)

#
# add Windows installer related targets
#
if(WIN32)
	include(WindowsInstaller)
endif()

#
# package generation
#
INCLUDE(cmake/CPackDefinitions.cmake)



#
# display configuration information
#

MESSAGE("\n"
"Veyon build summary\n"
"--------------------\n"
"* Version                     : ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD} (${VERSION_STRING})\n"
"* Install prefix              : ${CMAKE_INSTALL_PREFIX}\n"
"* Library directory           : ${CMAKE_INSTALL_PREFIX}/${VEYON_LIB_DIR}\n"
"* Plugin directory            : ${CMAKE_INSTALL_PREFIX}/${VEYON_INSTALL_PLUGIN_DIR}\n"
"* Build type                  : ${CMAKE_BUILD_TYPE}\n"
"* Build platform              : ${CMAKE_SYSTEM_PROCESSOR}\n"
"* Compile flags               : ${CMAKE_C_FLAGS} (CXX: ${CMAKE_CXX_FLAGS})\n"
)

IF(VEYON_BUILD_ANDROID)
	INCLUDE(AndroidDeployQt)
	SET(CMAKE_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
	SET(_CMAKE_ANDROID_DIR "${CMAKE_ANDROID_DIR}")
	SET(ANDROID_INSTALL_DIR "${CMAKE_BINARY_DIR}/install")
	SET(ANDROID_EXTRA_PLUGINS ${ANDROID_INSTALL_DIR}/${VEYON_LIB_DIR}/veyon/ ${QT_DIR}/lib/qca-qt5/crypto)
	FILE(GLOB ANDROID_EXTRA_LIBS ${ANDROID_INSTALL_DIR}/${VEYON_LIB_DIR}/*.so)
	LIST(APPEND ANDROID_EXTRA_LIBS "${ANDROID_SYSROOT_GENERIC}/libc++_shared.so")
	LIST(APPEND ANDROID_EXTRA_LIBS "${QT_DIR}/lib/libldap.so"
									"${QT_DIR}/lib/liblber.so"
									"${QT_DIR}/lib/libsasl2.so")
	androiddeployqt("veyon-master" "${ANDROID_ADDITIONAL_FIND_ROOT_PATH};${CMAKE_BINARY_DIR}/core")
	SET_TARGET_PROPERTIES(create-apk-veyon-master PROPERTIES ANDROID_APK_DIR "${CMAKE_ANDROID_DIR}")

	add_custom_target(prepare-apk
		COMMAND rm -rf ${ANDROID_INSTALL_DIR}
		COMMAND cd ${CMAKE_BINARY_DIR}/core && make DESTDIR=${ANDROID_INSTALL_DIR} install
		COMMAND cd ${CMAKE_BINARY_DIR}/plugins && make DESTDIR=${ANDROID_INSTALL_DIR} install
	)

	add_dependencies(create-apk-veyon-master prepare-apk)
ENDIF()
