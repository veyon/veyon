FILE(GLOB ica_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB ica_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ica.qrc)
QT4_WRAP_CPP(ica_MOC_out ${ica_INCLUDES})
QT4_ADD_RESOURCES(ica_RCC_out ${QRC_FILE})
QT4_TRANSLATIONS_FOR_QRC(${QRC_FILE} ${ica_SOURCES} ${ica_INCLUDES})
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
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/d3des.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver/vncauth.c
		${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc.c )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/x11 ${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc ${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncserver)
ADD_DEFINITIONS(-DVNCSHARED -DFOREVER -DNOREPEAT=0 -DNOPW=1 -DREMOTE_CONTROL=0 -DEXTERNAL_COMMANDS=0 -DFILEXFER=0 -DNOGUI -DSMALL_FOOTPRINT=3)

IF(ITALC_BUILD_LINUX)

	SET(platform_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc/appshare.c)

	LINK_LIBRARIES(${X11_LIBRARIES} ${X11_XTest_LIB} ${X11_Xfixes_LIB} ${X11_Xinerama_LIB} ${X11_Xdamage_LIB} ${X11_Xrandr_LIB})

ENDIF(ITALC_BUILD_LINUX)

IF(ITALC_BUILD_WIN32)

	ADD_DEFINITIONS(-DX11VNC_IPV6=0)
	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc ${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/omnithread ${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc ${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncclient)
	SET(platform_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/ultravnc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/HideDesktop.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/rfbRegion_win32.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vistahook.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncdesktopthread.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncdesktopsink.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/IPC.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncmenu.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncencoderre.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncserver.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncabout.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/black_layered.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/TextChat.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncsetauth.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncacceptdialog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/rfbUpdateTracker.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncencodehext.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/d3des.c
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncproperties.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/security.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/buildtime.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/Timer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncconndialog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncencoderCursor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncencoder.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vnclog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/translate.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncencodecorre.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncencodezrle.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vnchttpconnect.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncEncodeTight.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncservice.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncMultiMonitor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/inifile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncbuffer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/videodrivercheck.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/videodriver.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/read_write_ini.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vnctimedmsgbox.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncDesktopSW.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vnckeymap.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncOSVersion.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/winvnc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncauth.c
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncListDlg.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/stdhdrs.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncEncodeUltra.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncsockconnect.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncntlm.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncinsthandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncEncodeZlib.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncEncodeZlibHex.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/vncpropertiesPoll.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/helpers.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/service_motor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/service.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/CpuUsage.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/getinfo.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/winvnc/Dtwinver.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/omnithread/nt.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/Xregion/Region.c
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/DSMPlugin/DSMPlugin.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/common/clipboard.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/common/win32_helpers.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/rdr/ZlibOutStream.cxx
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/rdr/ZlibInStream.cxx
		${CMAKE_CURRENT_SOURCE_DIR}/win32/winvnc/dh.cpp
		)

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -fexceptions")
	LINK_LIBRARIES(-lnetapi32 -luserenv -lole32 -lversion -lgdi32 -limm32 -lwtsapi32)

	ADD_DEFINITIONS(-DULTRAVNC_ITALC_SUPPORT -DLIBVNCSERVER_NEED_INADDR_T)

ENDIF(ITALC_BUILD_WIN32)


ADD_DEFINITIONS(-DBUILD_ICA)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/x11/libvncclient)
LINK_LIBRARIES(ItalcCore)

ADD_EXECUTABLE(ica ${ica_SOURCES} ${ica_INCLUDES} ${ica_MOC_out} ${ica_RCC_out} ${x11vnc_SOURCES} ${platform_SOURCES})

