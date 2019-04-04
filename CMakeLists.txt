CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)

PROJECT(veyon)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE relwithdebinfo)
ENDIF()

IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0009 NEW)
	CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

INCLUDE(AddFileDependencies)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(GNUInstallDirs)
INCLUDE(CotireVeyon)

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
	SET(VERSION_MINOR 1)
	SET(VERSION_PATCH 8)
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
ELSE(WIN32)
    IF(APPLE)
	    SET(VEYON_BUILD_APPLE 1)
	ELSE(APPLE)
	    SET(VEYON_BUILD_LINUX 1)
	ENDIF(APPLE)
ENDIF(WIN32)

IF(WIN64)
    SET(VEYON_BUILD_WIN64 TRUE)
ENDIF(WIN64)

# set up library and plugin path variables
IF(CMAKE_INSTALL_LIBDIR)
    SET(VEYON_LIB_DIR "${CMAKE_INSTALL_LIBDIR}/veyon" CACHE INTERNAL "Veyon library directory")
ELSE()
    SET(VEYON_LIB_DIR lib/veyon CACHE INTERNAL "Veyon library directory")
ENDIF()

SET(VEYON_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/veyon")

IF(WIN32)
    SET(VEYON_PLUGIN_DIR "plugins")
    SET(VEYON_TRANSLATIONS_DIR "translations")
ELSE()
    SET(VEYON_PLUGIN_DIR "../${VEYON_LIB_DIR}")
    SET(VEYON_TRANSLATIONS_DIR "../share/veyon/translations")
ENDIF()


SET(VEYON_CORE_INCLUDE_DIR core/include)

# find required Qt5 modules
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5LinguistTools REQUIRED)

# find required libraries
FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(LZO REQUIRED)
FIND_PACKAGE(QCA REQUIRED)

# FindOpenSSL.cmake in recent versions of CMake will only find the DLLs instead
# of the import libraries if CYGWIN is not set
SET(CYGWIN TRUE)
FIND_PACKAGE(OpenSSL REQUIRED)
UNSET(CYGWIN)

# find Linux-specific packages
IF(VEYON_BUILD_LINUX)
    FIND_PACKAGE(PAM REQUIRED)
	INCLUDE(XdgInstall)
ENDIF()


### BEGIN: libvncclient configuration
CHECK_INCLUDE_FILES(endian.h LIBVNCSERVER_HAVE_ENDIAN_H)
CHECK_INCLUDE_FILES(limits.h LIBVNCSERVER_HAVE_LIMITS_H)
CHECK_INCLUDE_FILES(time.h LIBVNCSERVER_HAVE_TIME_H)
CHECK_INCLUDE_FILES(unistd.h LIBVNCSERVER_HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(ws2tcpip.h LIBVNCSERVER_HAVE_WS2TCPIP_H)
CHECK_INCLUDE_FILES(netinet/in.h LIBVNCSERVER_HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILES(sys/endian.h LIBVNCSERVER_HAVE_SYS_ENDIAN_H)
CHECK_INCLUDE_FILES(sys/socket.h LIBVNCSERVER_HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILES(sys/stat.h LIBVNCSERVER_HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(sys/time.h LIBVNCSERVER_HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/timeb.h LIBVNCSERVER_HAVE_SYS_TIMEB_H)
CHECK_INCLUDE_FILES(sys/types.h LIBVNCSERVER_HAVE_SYS_TYPES_H)

IF(LIBVNCSERVER_HAVE_SYS_SOCKET_H)
  LIST(APPEND CMAKE_EXTRA_INCLUDE_FILES "sys/socket.h")
ENDIF()

CHECK_TYPE_SIZE("pid_t" LIBVNCSERVER_PID_T)
CHECK_TYPE_SIZE("size_t" LIBVNCSERVER_SIZE_T)
CHECK_TYPE_SIZE("socklen_t" LIBVNCSERVER_SOCKLEN_T)

SET(_RFB_RFBCONFIG_H TRUE)
SET(LIBVNCSERVER_HAVE_LIBJPEG TRUE)
SET(LIBVNCSERVER_HAVE_LZO TRUE)
SET(LIBVNCSERVER_HAVE_LIBPNG TRUE)
SET(LIBVNCSERVER_HAVE_LIBZ TRUE)
SET(LIBVNCSERVER_HAVE_LIBSSL TRUE)
SET(LIBVNCSERVER_IPv6 TRUE)

IF(VEYON_BUILD_WIN32)
    SET(LIBVNCSERVER_NEED_INADDR_T TRUE)
ENDIF()

FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR}/rfb)
IF(NOT VEYON_BUILD_LINUX OR VEYON_X11VNC_EXTERNAL)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/3rdparty/libvncserver/rfb/rfbconfig.h.cmakein ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR}/rfb/rfbconfig.h @ONLY)
ENDIF()

### END: libvncclient configuration

# configure files for Veyon
SET(VEYONCONFIG ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR}/veyonconfig.h)

SET(IN_FILES ${VEYON_CORE_INCLUDE_DIR}/veyonconfig.h nsis/veyon.nsi core/builddata.qrc server/veyon-server.rc service/veyon-service.rc master/data/veyon-master.desktop master/veyon-master.rc configurator/data/veyon-configurator.desktop configurator/data/io.veyon.veyon-configurator.policy configurator/veyon-configurator.rc ctl/veyon-ctl.rc worker/veyon-worker.rc)
FOREACH(f ${IN_FILES})
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/${f}.in ${CMAKE_BINARY_DIR}/${f} @ONLY)
ENDFOREACH(f ${IN_FILES})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-exceptions -std=c++11 -fstack-protector-strong ${CXXFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fstack-protector-strong ${CFLAGS}")
IF(NOT VEYON_BUILD_WIN32)
    SET(SYS_INC SYSTEM)
ENDIF()

ADD_DEFINITIONS(-DQT_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x050600 -D_FORTIFY_SOURCE=2)

SET(3rdparty_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
SET(ultravnc_DIR ${3rdparty_DIR}/ultravnc)
SET(libvncserver_DIR ${3rdparty_DIR}/libvncserver)
SET(x11vnc_DIR ${3rdparty_DIR}/x11vnc)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/${VEYON_CORE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR} ${libvncserver_DIR})
INCLUDE_DIRECTORIES(${SYS_INC} ${ZLIB_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${QCA_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${LZO_INCLUDE_DIR})

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/core)

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
ADD_SUBDIRECTORY(ctl)
ADD_SUBDIRECTORY(worker)
ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(translations)

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
			COMMAND cp core/veyon-core.dll server/veyon-server.exe service/veyon-service.exe plugins/vncserver/ultravnc-builtin/vnchooks/vnchooks.dll master/veyon-master.exe configurator/veyon-configurator.exe ctl/veyon-ctl.exe worker/veyon-worker.exe ${TMP}
			COMMAND mkdir -p ${TMP}/plugins
			COMMAND cp plugins/*/*.dll ${TMP}/plugins/
			COMMAND cp plugins/*/*/*.dll ${TMP}/plugins/
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
			COMMAND cp ${DLLDIR}/Qt5Core.dll ${DLLDIR}/Qt5Gui.dll ${DLLDIR}/Qt5Widgets.dll ${DLLDIR}/Qt5Network.dll ${TMP}
			COMMAND mkdir -p ${TMP}/platforms
			COMMAND cp ${DLLDIR_LIB}/qt5/plugins/platforms/qwindows.dll ${TMP}/platforms
			COMMAND ${STRIP} ${TMP}/*.dll ${TMP}/*.exe ${TMP}/plugins/*.dll ${TMP}/platforms/*.dll ${TMP}/crypto/*.dll
			COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${TMP}
			COMMAND cp ${CMAKE_SOURCE_DIR}/COPYING ${TMP}/LICENSE.TXT
			COMMAND cp ${CMAKE_SOURCE_DIR}/README.md ${TMP}/README.TXT
			COMMAND todos ${TMP}/*.txt ${TMP}/*.TXT
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
"* Library & plugin directory  : ${CMAKE_INSTALL_PREFIX}/${VEYON_LIB_DIR}\n"
"* Build type                  : ${CMAKE_BUILD_TYPE}\n"
"* Build platform              : ${CMAKE_SYSTEM_PROCESSOR}\n"
"* Compile flags               : ${CMAKE_C_FLAGS} (CXX: ${CMAKE_CXX_FLAGS})\n"
)

