include(BuildVeyonPlugin)

# This plugin requires Qt 6 (uses Qt6::DBus) and PipeWire
if(NOT QT_MAJOR_VERSION EQUAL 6)
	return()
endif()

find_package(PipeWire)
find_package(Qt6 COMPONENTS DBus)

get_property(HAVE_LIBVNCCLIENT GLOBAL PROPERTY HAVE_LIBVNCCLIENT)
if(HAVE_LIBVNCCLIENT)
	find_package(LibVNCServer 0.9.8)
endif()

if(PipeWire_FOUND AND Qt6DBus_FOUND AND LibVNCServer_FOUND)

	build_veyon_plugin(pipewire-vnc-server
		NAME PipeWireVncServer
		SOURCES
		PipeWireVncServer.cpp
		PipeWireVncServer.h
		PipeWireFramebuffer.cpp
		PipeWireFramebuffer.h
		PortalSession.cpp
		PortalSession.h
	)

	target_link_libraries(pipewire-vnc-server
		PRIVATE
			PipeWire::PipeWire
			Qt6::DBus
			LibVNC::LibVNCServer
	)

	target_compile_options(pipewire-vnc-server PRIVATE -Wno-parentheses)

	install(
		PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/veyon-preauth-kde.sh"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/veyon"
		COMPONENT pipewire-vnc-server
	)

	if(VEYON_BUILD_LINUX)
		# Install the XDG .desktop file so xdg-desktop-portal can resolve the
		# well-known bus name "io.veyon.Veyon.Server" to the display name
		# "Veyon Server" for the KDE permission dialog.
		install(
			FILES "${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.Veyon.Server.desktop"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
			COMPONENT pipewire-vnc-server
		)

		# Install the D-Bus session service activation file so dbus-daemon (and
		# xdg-desktop-portal) can associate the well-known bus name with the
		# veyon-server binary and resolve the application identifier correctly.
		install(
			FILES "${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.Veyon.Server.service"
			DESTINATION "${CMAKE_INSTALL_DATADIR}/dbus-1/services"
			COMPONENT pipewire-vnc-server
		)
	endif()

endif()
