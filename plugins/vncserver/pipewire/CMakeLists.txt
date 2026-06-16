include(BuildVeyonPlugin)
include(BuildVeyonApplication)

find_package(PipeWire)
find_package(Qt${QT_MAJOR_VERSION} COMPONENTS DBus)

if(PipeWire_FOUND AND Qt${QT_MAJOR_VERSION}DBus_FOUND)

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

	# veyon-portal-helper: out-of-process executable that performs all D-Bus
	# portal interactions under the session user's credentials.  The main
	# veyon-server plugin (pipewire-vnc-server) launches this helper and
	# communicates with it via a Unix SOCK_SEQPACKET socket pair using the
	# protocol defined in PortalHelperProtocol.h.
	build_veyon_application(veyon-portal-helper
		SOURCES
		portal_helper_main.cpp
		PortalSession.cpp
		PortalSession.h
		${DBUS_GENERATED_SOURCES}
	)

	# The helper sources live in the plugin directory, not a src/ subdirectory,
	# so add the plugin source directory to the include path explicitly.
	target_include_directories(veyon-portal-helper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

	target_link_libraries(veyon-portal-helper PRIVATE
		Qt${QT_MAJOR_VERSION}::DBus
	)

	# pipewire-vnc-server plugin: D-Bus-free main-process side.
	# All D-Bus work is delegated to veyon-portal-helper via PortalHelperProcess.
	build_veyon_plugin(pipewire-vnc-server
		NAME PipeWireVncServer
		SOURCES
		PipeWireVncServer.cpp
		PipeWireVncServer.h
		PipeWireFramebuffer.cpp
		PipeWireFramebuffer.h
		PortalHelperProcess.cpp
		PortalHelperProcess.h
		PortalHelperProtocol.h
	)

	target_link_libraries(pipewire-vnc-server PRIVATE
		PipeWire::PipeWire
	)

	if(WITH_BUNDLED_LIBVNC)
		target_link_libraries(pipewire-vnc-server PRIVATE vncserver)
	else()
		target_link_libraries(pipewire-vnc-server PRIVATE LibVNC::LibVNCServer)
	endif()

	target_compile_options(pipewire-vnc-server PRIVATE -Wno-parentheses)

	install(
		PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/veyon-preauth-kde.sh"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/veyon"
		COMPONENT pipewire-vnc-server
	)
endif()
