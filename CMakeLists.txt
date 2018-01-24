CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

PROJECT(veyon)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
SET(CMAKE_BUILD_TYPE relwithdebinfo)

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
INCLUDE(FindPkgConfig)
INCLUDE(GNUInstallDirs)

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
	IF(NOT CONTRIBUTORS)
		SET(CONTRIBUTORS "${CMAKE_BINARY_DIR}/CONTRIBUTORS")
		EXECUTE_PROCESS(COMMAND "${GIT_EXECUTABLE}" shortlog -sne
						COMMAND cut -c8-
						COMMAND grep -v root
						OUTPUT_FILE "${CONTRIBUTORS}"
						WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
						TIMEOUT 10)
		FILE(RELATIVE_PATH CONTRIBUTORS ${CMAKE_BINARY_DIR}/core ${CONTRIBUTORS})
	ENDIF()

ENDIF()

# can't retrieve version information as not building from Git repository?
IF(NOT VERSION_STRING)
	SET(VERSION_MAJOR 4)
	SET(VERSION_MINOR 0)
	SET(VERSION_PATCH 3)
	SET(VERSION_BUILD 0)
	SET(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
ELSE()
	# remove leading character from tag name
	STRING(REPLACE "v" "" VERSION_STRING "${VERSION_STRING}")
ENDIF()


INCLUDE(DetectMachine)

SET(VEYON_CORE_INCLUDE_DIR core/include)

IF(VEYON_BUILD_LINUX)
	FIND_PACKAGE(PAM REQUIRED)
	INCLUDE(XdgInstall)
ENDIF()

SET(CMAKE_REQUIRED_LIBRARIES "-lm")

# check for required Qt5 modules

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5LinguistTools REQUIRED)


# find libraries
FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(LZO REQUIRED)
FIND_PACKAGE(QCA REQUIRED)
IF(VEYON_BUILD_WIN32)
	SET(OPENSSL_LIBRARIES -L${MINGW_PREFIX}/bin -leay32)
ELSE(VEYON_BUILD_WIN32)
	FIND_PACKAGE(OpenSSL REQUIRED)
ENDIF(VEYON_BUILD_WIN32)
FIND_PACKAGE(PNG REQUIRED)

# check for required UNIX headers
CHECK_INCLUDE_FILES(arpa/inet.h VEYON_HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILES(errno.h VEYON_HAVE_ERRNO_H)
CHECK_INCLUDE_FILES(netinet/in.h VEYON_HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILES(pwd.h VEYON_HAVE_PWD_H)
CHECK_INCLUDE_FILES(unistd.h VEYON_HAVE_UNISTD_H)

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

SET(IN_FILES ${VEYON_CORE_INCLUDE_DIR}/veyonconfig.h nsis/veyon.nsi core/builddata.qrc service/veyon-service.rc master/data/veyon-master.desktop master/veyon-master.rc configurator/data/veyon-configurator.desktop configurator/data/io.veyon.veyon-configurator.policy configurator/veyon-configurator.rc ctl/veyon-ctl.rc worker/veyon-worker.rc)
FOREACH(f ${IN_FILES})
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/${f}.in ${CMAKE_BINARY_DIR}/${f} @ONLY)
ENDFOREACH(f ${IN_FILES})

IF(VEYON_BUILD_WIN32)
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0600)
ENDIF()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fno-exceptions -std=c++11 ${CXXFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${CFLAGS}")
IF(VEYON_BUILD_WIN32)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes -Wno-pragmas")
ELSE()
	SET(OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES} -lcrypto)
ENDIF(VEYON_BUILD_WIN32)

ADD_DEFINITIONS(-DQT_DEPRECATED_WARNINGS -DQT_DISABLE_DEPRECATED_BEFORE=0x050600)

SET(3rdparty_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
SET(ultravnc_DIR ${3rdparty_DIR}/ultravnc)
SET(libvncserver_DIR ${3rdparty_DIR}/libvncserver)
SET(x11vnc_DIR ${3rdparty_DIR}/x11vnc)

INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${QCA_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${LZO_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/${VEYON_CORE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${libvncserver_DIR})

LINK_DIRECTORIES(${CMAKE_INSTALL_PREFIX}/core ${CMAKE_BINARY_DIR}/core)
LINK_LIBRARIES(${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENSSL_LIBRARIES} ${QCA_LIBRARY})

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# make sub-directories
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(service)
ADD_SUBDIRECTORY(master)
ADD_SUBDIRECTORY(configurator)
ADD_SUBDIRECTORY(ctl)
ADD_SUBDIRECTORY(worker)
ADD_SUBDIRECTORY(plugins)

INSTALL()

#
# rules for building localizations
#
FILE(GLOB veyon_LOCALES core/resources/*.ts)
SET(ts_targets "")
SET(qm_targets "")
FILE(GLOB_RECURSE veyon_SOURCES ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.ui)
STRING(REGEX REPLACE "${CMAKE_SOURCE_DIR}/3rdparty[^;]+;?" "" veyon_SOURCES "${veyon_SOURCES}")

ADD_CUSTOM_TARGET(download-translations)

FOREACH(_ts_file ${veyon_LOCALES})
	STRING(REPLACE "${CMAKE_SOURCE_DIR}/core/resources/" "" _ts_target "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_file "${_ts_file}")
	STRING(REPLACE ".ts" ".qm" _qm_target "${_ts_target}")
	ADD_CUSTOM_TARGET(${_ts_target} COMMAND ${Qt5_LUPDATE_EXECUTABLE} -I${CMAKE_SOURCE_DIR}/core/include -locations none -no-obsolete ${veyon_SOURCES} -ts ${_ts_file})
	# add command and target for generating/updating QM file if TS file is newer or no QM file exists yet
	ADD_CUSTOM_COMMAND(OUTPUT ${_qm_file} COMMAND ${Qt5_LRELEASE_EXECUTABLE} ${_ts_file} -qm ${_qm_file} DEPENDS ${_ts_file})
	ADD_CUSTOM_TARGET(${_qm_target} ALL DEPENDS ${_qm_file})

	LIST(APPEND ts_targets "${_ts_target}")
	LIST(APPEND qm_targets "${_qm_target}")

	# add target for downloading translation file from Transifex
	STRING(REPLACE ".ts" "" _translation "${_ts_target}")
	IF(NOT ${_translation} STREQUAL "veyon")
		SET(_download_translation_target "download-${_translation}")
		ADD_CUSTOM_TARGET(${_download_translation_target} COMMAND curl --user api:$ENV{TRANSIFEX_API_TOKEN} -o ${_ts_file} -X GET "https://www.transifex.com/api/2/project/veyon/resource/veyonts/translation/${_translation}/?mode=default\\&file")
		ADD_DEPENDENCIES(download-translations ${_download_translation_target})
	ENDIF()
ENDFOREACH(_ts_file ${veyon_LOCALES})

ADD_CUSTOM_TARGET(update-locales)
FOREACH(_item ${ts_targets})
	ADD_DEPENDENCIES(update-locales ${_item})
ENDFOREACH(_item ${ts_targets})

ADD_CUSTOM_TARGET(GenerateTranslationFiles)
FOREACH(_item ${qm_targets})
	ADD_DEPENDENCIES(GenerateTranslationFiles ${_item})
ENDFOREACH(_item ${qm_targets})


#
# add target for generating Windows installer
#

IF(WIN32)
	SET(InterceptionZip ${CMAKE_BINARY_DIR}/Interception.zip)
	SET(InterceptionInstaller ${CMAKE_BINARY_DIR}/install-interception.exe)
	IF (NOT EXISTS "${MY_DOWNLOAD_PATH}")
		FILE(DOWNLOAD "https://github.com/oblitum/Interception/releases/download/v1.0.1/Interception.zip" ${InterceptionZip} SHOW_PROGRESS EXPECTED_HASH SHA256=e9da708d334d1b5d9369ddb57dc8b63b2da1f5fe3ce9d14a24aa95d140d1bfbe)
	ENDIF()

	SET(TMP "veyon-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

	ADD_CUSTOM_TARGET(win-nsi
			COMMAND make
			COMMAND rm -rf ${TMP}
			COMMAND mkdir -p ${TMP}
			COMMAND unzip -j -d ${TMP} ${InterceptionZip} Interception/command\ line\ installer/install-interception.exe
			COMMAND cp core/veyon-core.dll service/veyon-service.exe plugins/vncserver/ultravnc-builtin/vnchooks/vnchooks.dll master/veyon-master.exe configurator/veyon-configurator.exe ctl/veyon-ctl.exe worker/veyon-worker.exe ${TMP}
			COMMAND mkdir -p ${TMP}/plugins
			COMMAND cp plugins/*/*.dll ${TMP}/plugins/
			COMMAND cp plugins/*/*/*.dll ${TMP}/plugins/
			COMMAND ${STRIP} ${TMP}/*.dll ${TMP}/*.exe ${TMP}/plugins/*
			COMMAND cp -r ${CMAKE_SOURCE_DIR}/doc ${TMP}
			COMMAND cp -r ${CMAKE_SOURCE_DIR}/contrib ${TMP}
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
"* Library & plugin directory  : ${CMAKE_INSTALL_PREFIX}/${LIB_DIR}\n"
"* Build type                  : ${CMAKE_BUILD_TYPE}\n"
"* Platform                    : ${CMAKE_SYSTEM_PROCESSOR} (${Machine})\n"
"* Compile flags               : ${CMAKE_C_FLAGS} (CXX: ${CMAKE_CXX_FLAGS})\n"
)

