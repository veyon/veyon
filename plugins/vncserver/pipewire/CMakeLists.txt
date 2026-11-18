include(BuildVeyonPlugin)

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

	build_veyon_plugin(pipewire-vnc-server
		NAME PipeWireVncServer
		SOURCES
		QuickConfiguration.cpp
		PipeWireVncServer.cpp
		PipeWireVncServer.h
		PipeWireVncConfiguration.h
		PipeWireVncConfigurationWidget.cpp
		PipeWireVncConfigurationWidget.h
		PipeWireVncConfigurationWidget.ui
		PipeWireFramebuffer.cpp
		PipeWireFramebuffer.h
		PortalSession.cpp
		PortalSession.h
		PortalInputDispatcher.h
		${DBUS_GENERATED_SOURCES}
	)

	target_link_libraries(pipewire-vnc-server PRIVATE
		PipeWire::PipeWire
		Qt${QT_MAJOR_VERSION}::DBus
	)

	if(WITH_BUNDLED_LIBVNC)
		target_link_libraries(pipewire-vnc-server PRIVATE vncserver)
	else()
		target_link_libraries(pipewire-vnc-server PRIVATE LibVNC::LibVNCServer)
	endif()

	target_compile_options(pipewire-vnc-server PRIVATE -Wno-parentheses)

	if(WITH_TESTS)
		add_executable(PortalSessionTest
			${CMAKE_SOURCE_DIR}/tests/unit/PortalSessionTest.cpp
			PortalSession.cpp PortalSession.h ${DBUS_GENERATED_SOURCES})
		target_include_directories(PortalSessionTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
		target_link_libraries(PortalSessionTest PRIVATE veyon-core Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::DBus)
		add_test(NAME PortalSessionTest COMMAND PortalSessionTest)
		set_tests_properties(PortalSessionTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
	endif()

	install(
		PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/veyon-preauth-kde.sh"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/veyon"
		COMPONENT pipewire-vnc-server
	)
endif()
