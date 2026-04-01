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

endif()
