if(WITH_TESTS AND VEYON_BUILD_LINUX)

	find_package(X11 REQUIRED)

	#
	# LinuxKeyboardMappingTest: tests keysym → linux keycode mapping tables
	# and UTF-8 → linux keycode conversion.
	#
	add_executable(LinuxKeyboardMappingTest
		LinuxKeyboardMappingTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxKeyboardInput.cpp
		${CMAKE_SOURCE_DIR}/3rdparty/libfakekey/src/libfakekey.c
	)

	target_include_directories(LinuxKeyboardMappingTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux
		${CMAKE_SOURCE_DIR}/3rdparty/libfakekey
		${CMAKE_SOURCE_DIR}/core/src
		${CMAKE_SOURCE_DIR}/core/src/Configuration
	)

	target_link_libraries(LinuxKeyboardMappingTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
		${X11_LIBRARIES}
		${X11_XTest_LIB}
	)

	add_test(NAME LinuxKeyboardMappingTest COMMAND LinuxKeyboardMappingTest)

	#
	# PipeWireVncServerTest: tests BGRx→RGB conversion, damage rect clamping,
	# and portal session token/path generation.
	#
	add_executable(PipeWireVncServerTest
		PipeWireVncServerTest.cpp
	)

	target_link_libraries(PipeWireVncServerTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME PipeWireVncServerTest COMMAND PipeWireVncServerTest)

	#
	# PortalInputDispatcherTest, PortalSessionTest: input order of the PipeWire VNC server's portal
	# session against fake portal services; only on a private session bus (dbus-run-session), never
	# on the desktop's bus
	#
	find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session)
	if(TARGET pipewire-vnc-server AND DBUS_RUN_SESSION_EXECUTABLE)
		if(WITH_QT6)
			find_package(Qt6 COMPONENTS DBus REQUIRED)
		else()
			find_package(Qt5DBus REQUIRED)
		endif()

		set(PIPEWIRE_VNC_SOURCE_DIR ${CMAKE_SOURCE_DIR}/plugins/vncserver/pipewire)

		add_executable(PortalInputDispatcherTest
			PortalInputDispatcherTest.cpp
			${PIPEWIRE_VNC_SOURCE_DIR}/PortalInputDispatcher.cpp
		)

		target_include_directories(PortalInputDispatcherTest PRIVATE ${PIPEWIRE_VNC_SOURCE_DIR})

		target_link_libraries(PortalInputDispatcherTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
			Qt${QT_MAJOR_VERSION}::DBus
			veyon-core
		)

		add_test(NAME PortalInputDispatcherTest
			COMMAND ${DBUS_RUN_SESSION_EXECUTABLE} -- $<TARGET_FILE:PortalInputDispatcherTest>)
		set_tests_properties(PortalInputDispatcherTest PROPERTIES
			ENVIRONMENT VEYON_TEST_PRIVATE_SESSION_BUS=1
			TIMEOUT 60)

		set(PORTAL_DBUS_INTERFACE_SOURCES)
		qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
			${PIPEWIRE_VNC_SOURCE_DIR}/dbus/org.freedesktop.portal.RemoteDesktop.xml
			OrgFreedesktopPortalRemoteDesktop
		)
		qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
			${PIPEWIRE_VNC_SOURCE_DIR}/dbus/org.freedesktop.portal.ScreenCast.xml
			OrgFreedesktopPortalScreenCast
		)
		qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
			${PIPEWIRE_VNC_SOURCE_DIR}/dbus/org.freedesktop.portal.Request.xml
			OrgFreedesktopPortalRequest
		)
		qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
			${PIPEWIRE_VNC_SOURCE_DIR}/dbus/org.freedesktop.portal.Session.xml
			OrgFreedesktopPortalSession
		)

		add_executable(PortalSessionTest
			PortalSessionTest.cpp
			${PIPEWIRE_VNC_SOURCE_DIR}/PortalInputDispatcher.cpp
			${PIPEWIRE_VNC_SOURCE_DIR}/PortalSession.cpp
			${PORTAL_DBUS_INTERFACE_SOURCES}
		)

		target_include_directories(PortalSessionTest PRIVATE
			${PIPEWIRE_VNC_SOURCE_DIR}
			${CMAKE_CURRENT_BINARY_DIR}
		)

		target_link_libraries(PortalSessionTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
			Qt${QT_MAJOR_VERSION}::DBus
			Qt${QT_MAJOR_VERSION}::Gui
			veyon-core
		)

		add_test(NAME PortalSessionTest
			COMMAND ${DBUS_RUN_SESSION_EXECUTABLE} -- $<TARGET_FILE:PortalSessionTest>)
		set_tests_properties(PortalSessionTest PROPERTIES
			ENVIRONMENT "VEYON_TEST_PRIVATE_SESSION_BUS=1;QT_QPA_PLATFORM=offscreen"
			TIMEOUT 60)
	endif()

	add_executable(VncProtocolRobustnessTest
		VncProtocolRobustnessTest.cpp
	)

	target_link_libraries(VncProtocolRobustnessTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME VncProtocolRobustnessTest COMMAND VncProtocolRobustnessTest)

	add_executable(FilesystemSafeJoinTest
		core/FilesystemSafeJoinTest.cpp
	)

	target_link_libraries(FilesystemSafeJoinTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME FilesystemSafeJoinTest COMMAND FilesystemSafeJoinTest)

	#
	# LinuxUserProcessTest: programs started by LinuxCoreFunctions::runProgramAsUser() outlive
	# the starting process without being killed by SIGPIPE
	#
	if(WITH_QT6)
		find_package(Qt6 COMPONENTS DBus REQUIRED)
	else()
		find_package(Qt5DBus REQUIRED)
	endif()

	add_executable(LinuxUserProcessTest
		LinuxUserProcessTest.cpp
	)

	target_include_directories(LinuxUserProcessTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux
		$<TARGET_PROPERTY:linux-platform,INCLUDE_DIRECTORIES>
	)

	target_compile_definitions(LinuxUserProcessTest PRIVATE
		$<TARGET_PROPERTY:linux-platform,COMPILE_DEFINITIONS>
	)

	target_link_libraries(LinuxUserProcessTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::DBus
		veyon-core
		linux-platform
	)

	add_test(NAME LinuxUserProcessTest COMMAND LinuxUserProcessTest)
	set_tests_properties(LinuxUserProcessTest PROPERTIES TIMEOUT 60)

	#
	# DesktopServicesFeatureTest: website URLs handled by the worker after serialization
	#
	if(TARGET desktopservices)
		add_executable(DesktopServicesFeatureTest
			DesktopServicesFeatureTest.cpp
		)

		add_dependencies(DesktopServicesFeatureTest linux-platform)

		target_include_directories(DesktopServicesFeatureTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/desktopservices)

		target_link_libraries(DesktopServicesFeatureTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Gui
			veyon-core
			desktopservices
		)

		add_test(NAME DesktopServicesFeatureTest COMMAND DesktopServicesFeatureTest)
		set_tests_properties(DesktopServicesFeatureTest PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)
	endif()

endif()
