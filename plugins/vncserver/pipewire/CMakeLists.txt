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

	# loaded by veyon-server: starts and supervises veyon-capture-helper (02-ipc.md §1a.5)
	build_veyon_plugin(pipewire-vnc-server
		NAME PipeWireVncServer
		SOURCES
		CaptureHelperCredentials.cpp
		CaptureHelperCredentials.h
		CaptureHelperProcess.cpp
		CaptureHelperProcess.h
		PipeWireVncServer.cpp
		PipeWireVncServer.h
		PipeWireVncConfiguration.cpp
		PipeWireVncConfiguration.h
		PipeWireVncConfigurationWidget.cpp
		PipeWireVncConfigurationWidget.h
		PipeWireVncConfigurationWidget.ui
	)

	# every portal call and the PipeWire stream run in this separate, dumpable process since
	# xdg-desktop-portal rejects the non-dumpable veyon-server (spike S7)
	build_veyon_application(veyon-capture-helper
		SOURCES
		VeyonCaptureHelper.cpp
		CaptureHelperCredentials.cpp
		CaptureHelperCredentials.h
		PipeWireCaptureServer.cpp
		PipeWireCaptureServer.h
		PipeWireFramebuffer.cpp
		PipeWireFramebuffer.h
		PipeWireVncConfiguration.cpp
		PipeWireVncConfiguration.h
		PortalSession.cpp
		PortalSession.h
		${DBUS_GENERATED_SOURCES}
	)

	# next to veyon-server, where CaptureHelperProcess::defaultHelperFilePath() looks for it, also
	# when running from the build tree
	set_target_properties(veyon-capture-helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/server)

	target_link_libraries(veyon-capture-helper PRIVATE
		PipeWire::PipeWire
		Qt${QT_MAJOR_VERSION}::DBus
		Qt${QT_MAJOR_VERSION}::Gui
	)

	if(WITH_BUNDLED_LIBVNC)
		target_link_libraries(veyon-capture-helper PRIVATE vncserver)
	else()
		target_link_libraries(veyon-capture-helper PRIVATE LibVNC::LibVNCServer)
	endif()

	target_compile_options(veyon-capture-helper PRIVATE -Wno-parentheses)

	install(
		PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/veyon-preauth-kde.sh"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/veyon"
		COMPONENT pipewire-vnc-server
	)
endif()
