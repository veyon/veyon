cmake_minimum_required(VERSION 3.1.0)

project(veyon)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(VEYON_DEBUG TRUE)
elseif(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE relwithdebinfo)
endif()

if(VEYON_DEBUG)
	add_definitions(-DVEYON_DEBUG)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(COMMAND CMAKE_POLICY)
	cmake_policy(SET CMP0009 NEW)
	cmake_policy(SET CMP0020 NEW)
	cmake_policy(SET CMP0063 NEW)
	if(${CMAKE_VERSION} VERSION_GREATER "3.8.0")
		cmake_policy(SET CMP0069 NEW)
	endif()
	if(${CMAKE_VERSION} VERSION_GREATER "3.12.0")
		cmake_policy(SET CMP0075 NEW)
	endif()
	if(${CMAKE_VERSION} VERSION_GREATER "3.14.0")
		cmake_policy(SET CMP0083 NEW)
	endif()
endif()

include(AddFileDependencies)
include(CheckCSourceCompiles)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(GNUInstallDirs)
include(CotireVeyon)
include(ConfigureFiles)
include(SetDefaultTargetProperties)

find_package(Git)

if(GIT_FOUND)
	execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
					OUTPUT_STRIP_TRAILING_WHITESPACE
					OUTPUT_VARIABLE VERSION_STRING)
	string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION_STRING}")
	string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION_STRING}")
	string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_PATCH "${VERSION_STRING}")

	# determine build number to use in NSIS installer and resource files
	execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags
					COMMAND cut -d "-" -f2
					WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
					OUTPUT_STRIP_TRAILING_WHITESPACE
					OUTPUT_VARIABLE VERSION_BUILD)
	if(NOT VERSION_BUILD GREATER 0)
		set(VERSION_BUILD 0)
	endif()

	# Get list of all committers from git history, ordered by number of commits.
	# The CONTRIBUTORS file is used by AboutDialog. This information can be provided
	# with -DCONTRIBUTORS=/path/to/CONTRIBUTORS instead. For instance, to generate
	# this file for version 3.0.2, the command is:
	#   git shortlog -sne v3.0.2 | cut -c8-
	set(CONTRIBUTORS "${CMAKE_BINARY_DIR}/CONTRIBUTORS")
	if(NOT EXISTS "${CONTRIBUTORS}")
		execute_process(COMMAND "${GIT_EXECUTABLE}" shortlog -s d160d147165271516589c304cb1b8f5e48f8527d..HEAD
						COMMAND cut -c8-
						COMMAND sort -f
						OUTPUT_FILE "${CONTRIBUTORS}"
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
						TIMEOUT 10)
	endif()

endif()

# can't retrieve version information as not building from Git repository?
if(NOT VERSION_STRING)
	set(VERSION_MAJOR 4)
	set(VERSION_MINOR 99)
	set(VERSION_PATCH 0)
	set(VERSION_BUILD 0)
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
else()
	# remove leading character from tag name
	string(REPLACE "v" "" VERSION_STRING "${VERSION_STRING}")
endif()

# set up compiler version variable
string(REGEX REPLACE "\\.[0-9]$" "" COMPILER_VERSION_MAJOR_MINOR ${CMAKE_CXX_COMPILER_VERSION})


# set up basic platform variables
if(WIN32)
	set(VEYON_BUILD_WIN32 1)
endif()
if(APPLE)
	set(VEYON_BUILD_APPLE 1)
endif()
if(UNIX AND NOT ANDROID)
	set(VEYON_BUILD_LINUX 1)
endif()
if(ANDROID)
	set(VEYON_BUILD_ANDROID 1)
endif()

if(WIN64)
	set(VEYON_BUILD_WIN64 TRUE)
endif()

# set up library and plugin path variables
if(VEYON_BUILD_ANDROID)
	set(CMAKE_INSTALL_PREFIX "/")
	set(VEYON_LIB_DIR "lib")
	set(VEYON_INSTALL_PLUGIN_DIR "${VEYON_LIB_DIR}/veyon")
	set(VEYON_INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/veyon")
	set(VEYON_PLUGIN_DIR "")
	set(VEYON_TRANSLATIONS_DIR "/translations")
else()
	if(CMAKE_INSTALL_LIBDIR)
		set(VEYON_LIB_DIR "${CMAKE_INSTALL_LIBDIR}/veyon" CACHE INTERNAL "Veyon library directory")
	else()
		set(VEYON_LIB_DIR lib/veyon CACHE INTERNAL "Veyon library directory")
	endif()

	set(VEYON_INSTALL_PLUGIN_DIR "${VEYON_LIB_DIR}")
	set(VEYON_INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/veyon")

	if(WIN32)
		set(VEYON_PLUGIN_DIR "plugins")
		set(VEYON_TRANSLATIONS_DIR "translations")
	else()
		set(VEYON_PLUGIN_DIR "../${VEYON_LIB_DIR}")
		set(VEYON_TRANSLATIONS_DIR "../share/veyon/translations")
	endif()
endif()


set(VEYON_CORE_INCLUDE_DIR core/src)

# find required Qt5 modules
find_package(Qt5Core REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5QuickControls2 REQUIRED)
if(VEYON_BUILD_ANDROID)
find_package(Qt5AndroidExtras REQUIRED)
endif()
if(VEYON_DEBUG)
find_package(Qt5Test REQUIRED)
set(VEYON_DEBUG_LIBRARIES Qt5::Test)
endif()

# find required libraries
find_package(QCA REQUIRED)

# FindOpenSSL.cmake in recent versions of CMake will only find the DLLs instead
# of the import libraries if CYGWIN is not set
set(CYGWIN TRUE)
find_package(OpenSSL REQUIRED)
unset(CYGWIN)

# find Linux-specific packages
if(VEYON_BUILD_LINUX)
	include(XdgInstall)
endif()

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

if(SANITIZE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fsanitize=undefined")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fsanitize=undefined")
endif()

set(VEYON_COMPILE_OPTIONS "-Wall;-Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong ${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong -fno-exceptions ${CXXFLAGS}")

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
message(WARNING "Not using -fvisibility=hidden and/or LTO with GCC < 6.0")
set(DISABLE_LTO TRUE)
else()
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif()

if(VEYON_BUILD_WIN32 OR VEYON_BUILD_WIN64)
set(DISABLE_LTO TRUE)
endif()

add_definitions(
	-DQT_DEPRECATED_WARNINGS
	-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00
	-D_FORTIFY_SOURCE=2
	-DQT_NO_CAST_FROM_ASCII
	-DQT_NO_CAST_TO_ASCII
	-DQT_NO_CAST_FROM_BYTEARRAY
	-DQT_NO_KEYWORDS
	-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
	-DQT_USE_QSTRINGBUILDER
	-DQT_STRICT_ITERATORS
)

file(GLOB_RECURSE IN_FILES RELATIVE ${CMAKE_SOURCE_DIR} "*config.h.in" "*.rc.in" "*.desktop.in" "*.policy.in" "*.service.in" "*.manifest.in" "*.nsi.in")
CONFIGURE_FILES(${IN_FILES})

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)

set(3rdparty_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
set(ultravnc_DIR ${3rdparty_DIR}/ultravnc)
set(libvncserver_DIR ${3rdparty_DIR}/libvncserver)
set(x11vnc_DIR ${3rdparty_DIR}/x11vnc)
set(libfakekey_DIR ${3rdparty_DIR}/libfakekey)

set(CMAKE_SKIP_BUILD_RPATH  FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${VEYON_LIB_DIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(VEYON_BUILD_ANDROID)
	include(AndroidDeployQt)
	set(_CMAKE_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
	set(ANDROID_INSTALL_DIR "${CMAKE_BINARY_DIR}/install")
	set(ANDROID_EXTRA_PLUGINS ${ANDROID_INSTALL_DIR}/${VEYON_LIB_DIR}/veyon/ ${QT_DIR}/lib/qca-qt5/crypto ${ANDROID_INSTALL_DIR}/jar)
	set(ANDROID_EXTRA_LIBS)
	list(APPEND ANDROID_EXTRA_LIBS "${ANDROID_SYSROOT_GENERIC}/libc++_shared.so")
	list(APPEND ANDROID_EXTRA_LIBS "${QT_DIR}/lib/libldap.so"
									"${QT_DIR}/lib/liblber.so"
									"${QT_DIR}/lib/libsasl2.so")

	add_custom_target(prepare-apk
		COMMAND rm -rf ${ANDROID_INSTALL_DIR}
		COMMAND cd ${CMAKE_BINARY_DIR}/core && make DESTDIR=${ANDROID_INSTALL_DIR} install
		COMMAND cd ${CMAKE_BINARY_DIR}/plugins && make DESTDIR=${ANDROID_INSTALL_DIR} install
	)
endif()

# make sub-directories
add_subdirectory(core)
add_subdirectory(server)
add_subdirectory(service)
add_subdirectory(master)
add_subdirectory(configurator)
add_subdirectory(cli)
add_subdirectory(worker)
add_subdirectory(plugins)
add_subdirectory(translations)

#
# add Windows installer related targets
#
if(WIN32)
	include(WindowsInstaller)
endif()

#
# package generation
#
include(cmake/CPackDefinitions.cmake)



#
# display configuration information
#

message("\n"
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
