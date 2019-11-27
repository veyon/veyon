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
	if(${CMAKE_VERSION} VERSION_GREATER "3.8.0")
		cmake_policy(SET CMP0069 NEW)
	endif()
	if(${CMAKE_VERSION} VERSION_GREATER "3.12.0")
		cmake_policy(SET CMP0075 NEW)
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
INCLUDE(SetDefaultTargetProperties)

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
	SET(VERSION_MINOR 99)
	SET(VERSION_PATCH 0)
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
	SET(VEYON_LIB_DIR "lib")
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
FIND_PACKAGE(Qt5Quick REQUIRED)
FIND_PACKAGE(Qt5QuickControls2 REQUIRED)
IF(VEYON_BUILD_ANDROID)
FIND_PACKAGE(Qt5AndroidExtras REQUIRED)
ENDIF()
IF(VEYON_DEBUG)
FIND_PACKAGE(Qt5Test REQUIRED)
SET(VEYON_DEBUG_LIBRARIES Qt5::Test)
ENDIF()

# find required libraries
FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(LZO REQUIRED)
FIND_PACKAGE(QCA REQUIRED)

SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads REQUIRED)

# FindOpenSSL.cmake in recent versions of CMake will only find the DLLs instead
# of the import libraries if CYGWIN is not set
SET(CYGWIN TRUE)
FIND_PACKAGE(OpenSSL REQUIRED)
UNSET(CYGWIN)

# find Linux-specific packages
IF(VEYON_BUILD_LINUX)
	INCLUDE(XdgInstall)
ENDIF()


### BEGIN: libvncserver configuration
INCLUDE(LibVNCServerIntegration)

SET(_RFB_RFBCONFIG_H TRUE)
SET(LIBVNCSERVER_HAVE_LIBJPEG TRUE)
SET(LIBVNCSERVER_HAVE_LZO TRUE)
SET(LIBVNCSERVER_HAVE_LIBPNG TRUE)
SET(LIBVNCSERVER_HAVE_LIBPTHREAD TRUE)
SET(LIBVNCSERVER_HAVE_LIBZ TRUE)
SET(LIBVNCSERVER_HAVE_LIBSSL TRUE)
SET(LIBVNCSERVER_ALLOW24BPP TRUE)
SET(LIBVNCSERVER_IPv6 TRUE)
### END: libvncserver configuration


IF(SANITIZE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fsanitize=undefined")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fsanitize=undefined")
ENDIF()

SET(VEYON_COMPILE_OPTIONS "-Wall;-Werror")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong ${CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong -fno-exceptions ${CXXFLAGS}")

IF(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
MESSAGE(WARNING "Not using -fvisibility=hidden and/or LTO with GCC < 6.0")
SET(DISABLE_LTO TRUE)
ELSE()
SET(CMAKE_C_VISIBILITY_PRESET hidden)
SET(CMAKE_CXX_VISIBILITY_PRESET hidden)
SET(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
ENDIF()

IF(VEYON_BUILD_WIN32 OR VEYON_BUILD_WIN64)
SET(DISABLE_LTO TRUE)
ENDIF()

ADD_DEFINITIONS(-DQT_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x051200 -D_FORTIFY_SOURCE=2 -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

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

IF(VEYON_BUILD_ANDROID)
	INCLUDE(AndroidDeployQt)
	SET(_CMAKE_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
	SET(ANDROID_INSTALL_DIR "${CMAKE_BINARY_DIR}/install")
	SET(ANDROID_EXTRA_PLUGINS ${ANDROID_INSTALL_DIR}/${VEYON_LIB_DIR}/veyon/ ${QT_DIR}/lib/qca-qt5/crypto ${ANDROID_INSTALL_DIR}/jar)
	SET(ANDROID_EXTRA_LIBS)
	LIST(APPEND ANDROID_EXTRA_LIBS "${ANDROID_SYSROOT_GENERIC}/libc++_shared.so")
	LIST(APPEND ANDROID_EXTRA_LIBS "${QT_DIR}/lib/libldap.so"
									"${QT_DIR}/lib/liblber.so"
									"${QT_DIR}/lib/libsasl2.so")

	add_custom_target(prepare-apk
		COMMAND rm -rf ${ANDROID_INSTALL_DIR}
		COMMAND cd ${CMAKE_BINARY_DIR}/core && make DESTDIR=${ANDROID_INSTALL_DIR} install
		COMMAND cd ${CMAKE_BINARY_DIR}/plugins && make DESTDIR=${ANDROID_INSTALL_DIR} install
	)
ENDIF()

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

# write libvncserver/libvncclient configuration header
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR}/rfb)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/3rdparty/libvncserver/rfb/rfbconfig.h.cmakein ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR}/rfb/rfbconfig.h @ONLY)


#
# add target for generating Windows installer
#

IF(WIN32)
	SET(TMP "veyon-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

	SET(DLLDIR "${MINGW_PREFIX}/bin")
	SET(DLLDIR_LIB "${MINGW_PREFIX}/lib")
	SET(DLLDIR_GCC "/usr/lib/gcc/${MINGW_TARGET}/${COMPILER_VERSION_MAJOR_MINOR}-win32")
	IF(VEYON_BUILD_WIN64)
		SET(DLL_GCC "libgcc_s_seh-1.dll")
		SET(DLL_DDENGINE "ddengine64.dll")
	ELSE()
		SET(DLL_GCC "libgcc_s_sjlj-1.dll")
		SET(DLL_DDENGINE "ddengine.dll")
	ENDIF()

	ADD_CUSTOM_TARGET(win-nsi
			COMMAND make
			COMMAND rm -rf ${TMP}
			COMMAND mkdir -p ${TMP}/interception
			COMMAND cp ${CMAKE_SOURCE_DIR}/3rdparty/interception/* ${TMP}/interception
			COMMAND cp ${CMAKE_SOURCE_DIR}/3rdparty/ddengine/${DLL_DDENGINE} ${TMP}
			COMMAND cp core/veyon-core.dll server/veyon-server.exe service/veyon-service.exe plugins/vncserver/ultravnc-builtin/vnchooks/vnchooks.dll master/veyon-master.exe configurator/veyon-configurator.exe cli/*.exe worker/veyon-worker.exe ${TMP}
			COMMAND mkdir -p ${TMP}/plugins
			COMMAND find plugins/ -name '*.dll' -exec cp '{}' ${TMP}/plugins/ '\;'
			COMMAND mv ${TMP}/plugins/lib*.dll ${TMP}
			COMMAND mkdir -p ${TMP}/translations
			COMMAND cp translations/*qm ${TMP}/translations/
			COMMAND cp ${DLLDIR}/libjpeg-62.dll ${TMP}
			COMMAND cp ${DLLDIR}/libpng16-16.dll ${TMP}
			COMMAND cp ${DLLDIR}/libcrypto-1_1*.dll ${DLLDIR}/libssl-1_1*.dll ${TMP}
			COMMAND cp ${DLLDIR}/libqca-qt5.dll ${TMP}
			COMMAND cp ${DLLDIR}/libsasl2-3.dll ${TMP}
			COMMAND cp ${DLLDIR}/libldap.dll ${DLLDIR}/liblber.dll ${TMP}
			COMMAND cp ${DLLDIR}/interception.dll ${TMP}
			COMMAND cp ${DLLDIR}/liblzo2-2.dll ${TMP}
			COMMAND cp ${DLLDIR_LIB}/zlib1.dll ${TMP}
			COMMAND cp ${DLLDIR_LIB}/libwinpthread-1.dll ${TMP}
			COMMAND cp ${DLLDIR_GCC}/libstdc++-6.dll ${TMP}
			COMMAND cp ${DLLDIR_GCC}/libssp-0.dll ${TMP}
			COMMAND cp ${DLLDIR_GCC}/${DLL_GCC} ${TMP}
			COMMAND mkdir -p ${TMP}/crypto
			COMMAND cp ${DLLDIR_LIB}/qca-qt5/crypto/libqca-ossl.dll ${TMP}/crypto
			COMMAND cp ${DLLDIR}/Qt5Core.dll ${DLLDIR}/Qt5Gui.dll ${DLLDIR}/Qt5Widgets.dll ${DLLDIR}/Qt5Network.dll ${DLLDIR}/Qt5Concurrent.dll ${DLLDIR}/Qt5Qml.dll ${DLLDIR}/Qt5Quick.dll ${DLLDIR}/Qt5QuickControls2.dll ${DLLDIR}/Qt5QuickTemplates2.dll ${TMP}
			COMMAND mkdir -p ${TMP}/platforms
			COMMAND cp ${DLLDIR_LIB}/qt5/plugins/platforms/qwindows.dll ${TMP}/platforms
			COMMAND mkdir -p ${TMP}/styles
			COMMAND cp ${DLLDIR_LIB}/qt5/plugins/styles/*.dll ${TMP}/styles
			COMMAND ${STRIP} ${TMP}/*.dll ${TMP}/*.exe ${TMP}/plugins/*.dll ${TMP}/platforms/*.dll ${TMP}/styles/*.dll ${TMP}/crypto/*.dll
			COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${TMP}
			COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${TMP}/LICENSE.TXT
			COMMAND cp ${CMAKE_SOURCE_DIR}/README.md ${TMP}/README.TXT
			COMMAND todos ${TMP}/*.TXT
			COMMAND cp nsis/veyon.nsi ${TMP}
			COMMAND makensis ${TMP}/veyon.nsi
			COMMAND mv ${TMP}/veyon-*setup.exe .
			#COMMAND rm -rf ${TMP}
		)
ENDIF()

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

