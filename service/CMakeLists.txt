FILE(GLOB service_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB service_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
QT5_WRAP_CPP(service_MOC_out ${service_INCLUDES})
#QT5_TRANSLATIONS_FOR_QRC(${QRC_FILE} ${service_SOURCES} ${service_INCLUDES})
ADD_DEFINITIONS(-DLIBVNCSERVER_PACKAGE_STRING="iTALC ${VERSION}")


SET(x11vnc_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/auth.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/cargs.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/corre.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/cursor.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/cutpaste.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/draw.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/font.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/hextile.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/httpd.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/main.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/rfbregion.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/rfbserver.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/rre.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/scale.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/selbox.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/sockets.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/stats.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/translate.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/ultra.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/zlib.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/zrle.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/zrleoutstream.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/zrlepalettehelper.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/tight.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/common/d3des.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/common/vncauth.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc.c )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/x11 ${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc ${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver ${CMAKE_CURRENT_SOURCE_DIR}/x11/common)
ADD_DEFINITIONS(-DVNCSHARED -DFOREVER -DNOREPEAT=0 -DNOPW=1 -DREMOTE_CONTROL=0 -DEXTERNAL_COMMANDS=0 -DFILEXFER=0 -DNOGUI -DSMALL_FOOTPRINT=3)

IF(ITALC_BUILD_LINUX)

	ADD_SUBDIRECTORY(helpers)

	SET(platform_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc/appshare.c)

	LINK_LIBRARIES(${X11_LIBRARIES} ${X11_XTest_LIB} ${X11_Xfixes_LIB} ${X11_Xinerama_LIB} ${X11_Xdamage_LIB} ${X11_Xrandr_LIB})

ENDIF(ITALC_BUILD_LINUX)

IF(ITALC_BUILD_WIN32)

	ADD_SUBDIRECTORY(win32)

	ADD_DEFINITIONS(-DX11VNC_IPV6=0 -DPTW32_STATIC_LIB -DTLS=__thread -DX11VNC_USLEEP -D__USE_MINGW_ANSI_STDIO=1)
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/win32 ${ULTRAVNC_DIR} ${ULTRAVNC_DIR}/winvnc ${ULTRAVNC_DIR}/winvnc/omnithread ${ULTRAVNC_DIR}/winvnc/winvnc ${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncclient)
	SET(platform_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/ultravnc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/ultravnc-italc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/vncntlm.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/HideDesktop.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/rfbRegion_win32.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vistahook.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncdesktopthread.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncdesktopsink.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/IPC.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncencoderre.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncserver.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncabout.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/black_layered.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncsetauth.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncacceptdialog.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/rfbUpdateTracker.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncencodehext.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/d3des.c
		${ULTRAVNC_DIR}/winvnc/winvnc/vncproperties.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/security.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/buildtime.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/Timer.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncconndialog.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncencoderCursor.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncencoder.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vnclog.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/translate.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncencodecorre.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncencodezrle.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vnchttpconnect.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncEncodeTight.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncservice.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncMultiMonitor.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/inifile.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncbuffer.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/videodrivercheck.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/videodriver.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/read_write_ini.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vnctimedmsgbox.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncDesktopSW.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vnckeymap.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncOSVersion.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/winvnc.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncauth.c
		${ULTRAVNC_DIR}/winvnc/winvnc/vncListDlg.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/stdhdrs.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncEncodeUltra.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncEncodeUltra2.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncsockconnect.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncinsthandler.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncEncodeZlib.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncEncodeZlibHex.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/vncpropertiesPoll.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/helpers.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/service_motor.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/service.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/CpuUsage.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/getinfo.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/Dtwinver.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/uvncUiAccess.cpp
		${ULTRAVNC_DIR}/winvnc/winvnc/blankmonitor.cpp
		${ULTRAVNC_DIR}/winvnc/omnithread/nt.cpp
		${ULTRAVNC_DIR}/DSMPlugin/DSMPlugin.cpp
		${ULTRAVNC_DIR}/common/Clipboard.cpp
		${ULTRAVNC_DIR}/common/win32_helpers.cpp
		${ULTRAVNC_DIR}/rdr/ZlibOutStream.cxx
		${ULTRAVNC_DIR}/rdr/ZlibInStream.cxx
		)

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -fexceptions")
	LINK_LIBRARIES(-luserenv -lole32 -lversion -lgdi32 -limm32 -lwinmm)

	ADD_DEFINITIONS(-DULTRAVNC_ITALC_SUPPORT -DLIBVNCSERVER_NEED_INADDR_T)

	SET(WINRC "${CMAKE_CURRENT_BINARY_DIR}/italc-service-rc.obj")
	ADD_CUSTOM_COMMAND(OUTPUT ${WINRC}
						COMMAND ${WINDRES}
							-I${CMAKE_CURRENT_SOURCE_DIR}
							-o${WINRC}
							-i${CMAKE_CURRENT_BINARY_DIR}/italc-service.rc
						DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/italc-service.rc)

ENDIF(ITALC_BUILD_WIN32)

IF(ITALC_BUILD_WIN64)
	ADD_DEFINITIONS(-D_X64)
ENDIF(ITALC_BUILD_WIN64)


ADD_DEFINITIONS(-DBUILD_ICA)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncclient)
LINK_LIBRARIES(-lpthread)

ADD_EXECUTABLE(italc-service ${service_SOURCES} ${service_INCLUDES} ${service_MOC_out} ${x11vnc_SOURCES} ${platform_SOURCES} ${WINRC})
TARGET_LINK_LIBRARIES(italc-service italc-core)

INSTALL(TARGETS italc-service RUNTIME DESTINATION bin)

IF(ITALC_BUILD_WIN32)
	SET_TARGET_PROPERTIES(italc-service PROPERTIES LINK_FLAGS -mwindows)
ENDIF(ITALC_BUILD_WIN32)

TARGET_LINK_LIBRARIES(italc-service
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
        Qt5::Xml
	)

