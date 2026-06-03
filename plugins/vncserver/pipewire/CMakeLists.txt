include(BuildVeyonPlugin)

find_package(PipeWire)
find_package(Qt${QT_MAJOR_VERSION} COMPONENTS DBus)

get_property(HAVE_LIBVNCCLIENT GLOBAL PROPERTY HAVE_LIBVNCCLIENT)
if(HAVE_LIBVNCCLIENT)
	find_package(LibVNCServer 0.9.8)
endif()

if(VEYON_USE_BUILTIN_LIBVNC OR NOT LibVNCServer_FOUND)
	include(LibVNCServerIntegration)

	set(libvncserver_SOURCES
		${libvncserver_DIR}/src/libvncserver/auth.c
		${libvncserver_DIR}/src/libvncserver/cargs.c
		${libvncserver_DIR}/src/libvncserver/corre.c
		${libvncserver_DIR}/src/libvncserver/cursor.c
		${libvncserver_DIR}/src/libvncserver/cutpaste.c
		${libvncserver_DIR}/src/libvncserver/draw.c
		${libvncserver_DIR}/src/libvncserver/font.c
		${libvncserver_DIR}/src/libvncserver/hextile.c
		${libvncserver_DIR}/src/libvncserver/httpd.c
		${libvncserver_DIR}/src/libvncserver/main.c
		${libvncserver_DIR}/src/libvncserver/rfbregion.c
		${libvncserver_DIR}/src/libvncserver/rfbserver.c
		${libvncserver_DIR}/src/libvncserver/rre.c
		${libvncserver_DIR}/src/libvncserver/scale.c
		${libvncserver_DIR}/src/libvncserver/selbox.c
		${libvncserver_DIR}/src/libvncserver/sockets.c
		${libvncserver_DIR}/src/libvncserver/stats.c
		${libvncserver_DIR}/src/libvncserver/translate.c
		${libvncserver_DIR}/src/libvncserver/ultra.c
		${libvncserver_DIR}/src/libvncserver/zlib.c
		${libvncserver_DIR}/src/libvncserver/zrle.c
		${libvncserver_DIR}/src/libvncserver/zrleoutstream.c
		${libvncserver_DIR}/src/libvncserver/zrlepalettehelper.c
		${libvncserver_DIR}/src/libvncserver/tight.c
		${libvncserver_DIR}/src/common/d3des.c
		${libvncserver_DIR}/src/common/sockets.c
		${libvncserver_DIR}/src/common/turbojpeg.c
		${libvncserver_DIR}/src/common/vncauth.c)
endif()

if(PipeWire_FOUND AND Qt${QT_MAJOR_VERSION}DBus_FOUND AND (LibVNCServer_FOUND OR VEYON_USE_BUILTIN_LIBVNC))

	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.RemoteDesktop.xml
		OrgFreedesktopPortalRemoteDesktop
	)
	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.ScreenCast.xml
		OrgFreedesktopPortalScreenCast
	)
	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.Request.xml
		OrgFreedesktopPortalRequest
	)
	qt_add_dbus_interface(DBUS_GENERATED_SOURCES
		dbus/org.freedesktop.portal.Session.xml
		OrgFreedesktopPortalSession
	)

	build_veyon_plugin(pipewire-vnc-server
		NAME PipeWireVncServer
		SOURCES
		PipeWireVncServer.cpp
		PipeWireVncServer.h
		PipeWireFramebuffer.cpp
		PipeWireFramebuffer.h
		PortalSession.cpp
		PortalSession.h
		${DBUS_GENERATED_SOURCES}
		${libvncserver_SOURCES}
	)

	target_link_libraries(pipewire-vnc-server PRIVATE
		PipeWire::PipeWire
		Qt${QT_MAJOR_VERSION}::DBus
	)

	if(LibVNCServer_FOUND)
		target_link_libraries(pipewire-vnc-server PRIVATE LibVNC::LibVNCServer)
	else()
		target_link_libraries(pipewire-vnc-server PRIVATE
			Threads::Threads
			PNG::PNG
			${ZLIB_LIBRARIES}
			${JPEG_LIBRARIES}
			${LZO_LIBRARIES})
		target_include_directories(pipewire-vnc-server PRIVATE
			${libvncserver_DIR}/libvncserver
			${libvncserver_DIR}/common)
	endif()

	target_compile_options(pipewire-vnc-server PRIVATE -Wno-parentheses)

	set_source_files_properties(${libvncserver_SOURCES} PROPERTIES
		COMPILE_FLAGS "-Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable"
		SKIP_PRECOMPILE_HEADERS TRUE)

	install(
		PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/veyon-preauth-kde.sh"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/veyon"
		COMPONENT pipewire-vnc-server
	)
endif()
