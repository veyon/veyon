FILE(GLOB service_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB service_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
QT5_WRAP_CPP(service_MOC_out ${service_INCLUDES})

IF(ITALC_BUILD_LINUX)

	ADD_SUBDIRECTORY(helpers)

	ADD_DEFINITIONS(-DLIBVNCSERVER_PACKAGE_STRING="iTALC ${VERSION}")
	ADD_DEFINITIONS(-DVNCSHARED -DFOREVER -DNOREPEAT=0 -DNOPW=1 -DREMOTE_CONTROL=0 -DEXTERNAL_COMMANDS=0 -DFILEXFER=0 -DNOGUI -DSMALL_FOOTPRINT=3)
	INCLUDE_DIRECTORIES(${libvncserver_DIR}/libvncserver ${libvncserver_DIR}/common ${3rdparty_DIR} ${x11vnc_DIR}/src)

	SET(libvncserver_SOURCES
               ${libvncserver_DIR}/libvncserver/auth.c
               ${libvncserver_DIR}/libvncserver/cargs.c
               ${libvncserver_DIR}/libvncserver/corre.c
               ${libvncserver_DIR}/libvncserver/cursor.c
               ${libvncserver_DIR}/libvncserver/cutpaste.c
               ${libvncserver_DIR}/libvncserver/draw.c
               ${libvncserver_DIR}/libvncserver/font.c
               ${libvncserver_DIR}/libvncserver/hextile.c
               ${libvncserver_DIR}/libvncserver/httpd.c
               ${libvncserver_DIR}/libvncserver/main.c
               ${libvncserver_DIR}/libvncserver/rfbregion.c
               ${libvncserver_DIR}/libvncserver/rfbserver.c
               ${libvncserver_DIR}/libvncserver/rre.c
               ${libvncserver_DIR}/libvncserver/scale.c
               ${libvncserver_DIR}/libvncserver/selbox.c
               ${libvncserver_DIR}/libvncserver/sockets.c
               ${libvncserver_DIR}/libvncserver/stats.c
               ${libvncserver_DIR}/libvncserver/translate.c
               ${libvncserver_DIR}/libvncserver/ultra.c
               ${libvncserver_DIR}/libvncserver/zlib.c
               ${libvncserver_DIR}/libvncserver/zrle.c
               ${libvncserver_DIR}/libvncserver/zrleoutstream.c
               ${libvncserver_DIR}/libvncserver/zrlepalettehelper.c
               ${libvncserver_DIR}/libvncserver/tight.c
               ${libvncserver_DIR}/common/d3des.c
               ${libvncserver_DIR}/common/turbojpeg.c
               ${libvncserver_DIR}/common/vncauth.c)

	SET(platform_SOURCES
		${libvncserver_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/x11/x11vnc-italc.c
		${x11vnc_DIR}/src/appshare.c)

	LINK_LIBRARIES(${X11_LIBRARIES} ${X11_XTest_LIB} ${X11_Xfixes_LIB} ${X11_Xinerama_LIB} ${X11_Xdamage_LIB} ${X11_Xrandr_LIB} ${X11_Xi2_LIB})

ENDIF(ITALC_BUILD_LINUX)

IF(ITALC_BUILD_WIN32)

	ADD_SUBDIRECTORY(win32)

	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/win32 ${ultravnc_DIR} ${ultravnc_DIR}/winvnc ${ultravnc_DIR}/winvnc/omnithread ${ultravnc_DIR}/winvnc/winvnc)
	SET(platform_SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/win32/ultravnc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/ultravnc-italc.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/vncntlm.cpp
		${ultravnc_DIR}/winvnc/winvnc/HideDesktop.cpp
		${ultravnc_DIR}/winvnc/winvnc/rfbRegion_win32.cpp
		${ultravnc_DIR}/winvnc/winvnc/vistahook.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncdesktopthread.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncdesktopsink.cpp
		${ultravnc_DIR}/winvnc/winvnc/IPC.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncencoderre.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncserver.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncabout.cpp
		${ultravnc_DIR}/winvnc/winvnc/black_layered.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncsetauth.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncacceptdialog.cpp
		${ultravnc_DIR}/winvnc/winvnc/rfbUpdateTracker.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncencodehext.cpp
		${ultravnc_DIR}/winvnc/winvnc/d3des.c
		${ultravnc_DIR}/winvnc/winvnc/vncproperties.cpp
		${ultravnc_DIR}/winvnc/winvnc/security.cpp
		${ultravnc_DIR}/winvnc/winvnc/buildtime.cpp
		${ultravnc_DIR}/winvnc/winvnc/Timer.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncconndialog.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncencoderCursor.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncencoder.cpp
		${ultravnc_DIR}/winvnc/winvnc/vnclog.cpp
		${ultravnc_DIR}/winvnc/winvnc/translate.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncencodecorre.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncencodezrle.cpp
		${ultravnc_DIR}/winvnc/winvnc/vnchttpconnect.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncEncodeTight.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncservice.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncMultiMonitor.cpp
		${ultravnc_DIR}/winvnc/winvnc/inifile.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncbuffer.cpp
		${ultravnc_DIR}/winvnc/winvnc/videodrivercheck.cpp
		${ultravnc_DIR}/winvnc/winvnc/videodriver.cpp
		${ultravnc_DIR}/winvnc/winvnc/read_write_ini.cpp
		${ultravnc_DIR}/winvnc/winvnc/vnctimedmsgbox.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncDesktopSW.cpp
		${ultravnc_DIR}/winvnc/winvnc/vnckeymap.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncOSVersion.cpp
		${ultravnc_DIR}/winvnc/winvnc/winvnc.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncauth.c
		${ultravnc_DIR}/winvnc/winvnc/vncListDlg.cpp
		${ultravnc_DIR}/winvnc/winvnc/stdhdrs.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncEncodeUltra.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncEncodeUltra2.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncsockconnect.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncinsthandler.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncEncodeZlib.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncEncodeZlibHex.cpp
		${ultravnc_DIR}/winvnc/winvnc/vncpropertiesPoll.cpp
		${ultravnc_DIR}/winvnc/winvnc/helpers.cpp
		${ultravnc_DIR}/winvnc/winvnc/service_motor.cpp
		${ultravnc_DIR}/winvnc/winvnc/service.cpp
		${ultravnc_DIR}/winvnc/winvnc/CpuUsage.cpp
		${ultravnc_DIR}/winvnc/winvnc/getinfo.cpp
		${ultravnc_DIR}/winvnc/winvnc/Dtwinver.cpp
		${ultravnc_DIR}/winvnc/winvnc/uvncUiAccess.cpp
		${ultravnc_DIR}/winvnc/winvnc/blankmonitor.cpp
		${ultravnc_DIR}/winvnc/omnithread/nt.cpp
		${ultravnc_DIR}/DSMPlugin/DSMPlugin.cpp
		${ultravnc_DIR}/common/Clipboard.cpp
		${ultravnc_DIR}/common/win32_helpers.cpp
		${ultravnc_DIR}/rdr/ZlibOutStream.cxx
		${ultravnc_DIR}/rdr/ZlibInStream.cxx
		)

	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings -Wno-unknown-pragmas -fexceptions")
	LINK_LIBRARIES(-luserenv -lole32 -lversion -lgdi32 -limm32 -lwinmm)

	ADD_DEFINITIONS(-DULTRAVNC_ITALC_SUPPORT)

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


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

ADD_EXECUTABLE(italc-service ${service_SOURCES} ${service_INCLUDES} ${service_MOC_out} ${platform_SOURCES} ${WINRC})
TARGET_LINK_LIBRARIES(italc-service italc-core)

INSTALL(TARGETS italc-service RUNTIME DESTINATION bin)

SET_SOURCE_FILES_PROPERTIES(${platform_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-result -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-deprecated-declarations")

IF(ITALC_BUILD_WIN32)
	SET_TARGET_PROPERTIES(italc-service PROPERTIES LINK_FLAGS -mwindows)
ENDIF(ITALC_BUILD_WIN32)

TARGET_LINK_LIBRARIES(italc-service
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
        Qt5::Xml
		-lpthread
	)

IF(ITALC_BUILD_LINUX)
	TARGET_LINK_LIBRARIES(italc-service ${X11_LIBRARIES} ${X11_XTest_LIB} ${X11_Xfixes_LIB} ${X11_Xinerama_LIB} ${X11_Xdamage_LIB} ${X11_Xrandr_LIB} ${X11_Xi2_LIB})
ENDIF()
