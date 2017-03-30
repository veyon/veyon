INCLUDE(BuildPlugin)

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

BUILD_PLUGIN(builtin-x11vnc-server
	BuiltinX11VncServer.cpp
	X11VncConfiguration.cpp
	X11VncConfigurationWidget.cpp
	x11vnc-italc.c
	${x11vnc_DIR}/src/appshare.c
	${libvncserver_SOURCES}
	MOCFILES
	BuiltinX11VncServer.h
	X11VncConfigurationWidget.h
	X11VncConfiguration.h
	FORMS X11VncConfigurationWidget.ui)

TARGET_LINK_LIBRARIES(builtin-x11vnc-server
	-lpthread
	${X11_LIBRARIES}
	${X11_XTest_LIB}
	${X11_Xfixes_LIB}
	${X11_Xinerama_LIB}
	${X11_Xdamage_LIB}
	${X11_Xrandr_LIB}
	${X11_Xi2_LIB})
