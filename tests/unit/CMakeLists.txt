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
	# PortalPointerMappingTest: VNC framebuffer positions to the coordinate space of a portal stream
	#
	add_executable(PortalPointerMappingTest
		PortalPointerMappingTest.cpp
	)

	target_include_directories(PortalPointerMappingTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/vncserver/pipewire)

	target_link_libraries(PortalPointerMappingTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME PortalPointerMappingTest COMMAND PortalPointerMappingTest)

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

	# redaction of sensitive feature arguments in logs, with the plugins of the build tree
	add_executable(FeatureMessageRedactionTest
		core/FeatureMessageRedactionTest.cpp
	)

	add_dependencies(FeatureMessageRedactionTest linux-platform usersessioncontrol demo remoteaccess filetransfer)

	target_link_libraries(FeatureMessageRedactionTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Widgets
		veyon-core
	)

	add_test(NAME FeatureMessageRedactionTest COMMAND FeatureMessageRedactionTest)
	set_tests_properties(FeatureMessageRedactionTest PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)

	# debug output of the WebAPI plugin never contains the session token or credentials
	if(WITH_WEBAPI)
		add_executable(WebApiLogRedactionTest
			core/WebApiLogRedactionTest.cpp
		)

		target_include_directories(WebApiLogRedactionTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/webapi)

		target_link_libraries(WebApiLogRedactionTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Concurrent
			veyon-core
		)

		add_test(NAME WebApiLogRedactionTest COMMAND WebApiLogRedactionTest)
	endif()

	#
	# UserIdentificationCacheTest: server-side cache and request collection of the IdentifyUser feature
	#
	add_executable(UserIdentificationCacheTest
		core/UserIdentificationCacheTest.cpp
	)

	target_link_libraries(UserIdentificationCacheTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME UserIdentificationCacheTest COMMAND UserIdentificationCacheTest)

	#
	# FileCollectTest: grouping values and output files of FileCollect
	#
	if(TARGET filetransfer)
		add_executable(FileCollectTest
			FileCollectTest.cpp
		)

		add_dependencies(FileCollectTest linux-platform)

		target_include_directories(FileCollectTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/filetransfer)

		target_link_libraries(FileCollectTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
			veyon-core
			filetransfer
		)

		add_test(NAME FileCollectTest COMMAND FileCollectTest)
	endif()

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
	# LinuxUserFunctionsTest: user environment (HOME etc.) from the password database
	#
	add_executable(LinuxUserFunctionsTest
		LinuxUserFunctionsTest.cpp
	)

	target_include_directories(LinuxUserFunctionsTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux
		$<TARGET_PROPERTY:linux-platform,INCLUDE_DIRECTORIES>
	)

	target_compile_definitions(LinuxUserFunctionsTest PRIVATE
		$<TARGET_PROPERTY:linux-platform,COMPILE_DEFINITIONS>
	)

	target_link_libraries(LinuxUserFunctionsTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::DBus
		veyon-core
		linux-platform
	)

	add_test(NAME LinuxUserFunctionsTest COMMAND LinuxUserFunctionsTest)

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

	# PluginLoadingTest: plugins with unresolved symbols (e.g. built for an older Veyon version) are
	# refused at load time and logged. Linux only: on Windows a DLL with an undefined symbol does not link.
	add_library(valid-test-plugin MODULE plugin-loading/ValidTestPlugin.cpp)
	target_link_libraries(valid-test-plugin PRIVATE Qt${QT_MAJOR_VERSION}::Core)

	# no set_default_target_properties(): it links with -no-undefined
	add_library(incompatible-test-plugin MODULE plugin-loading/IncompatibleTestPlugin.cpp)
	target_link_libraries(incompatible-test-plugin PRIVATE Qt${QT_MAJOR_VERSION}::Core)
	target_link_options(incompatible-test-plugin PRIVATE "-Wl,--allow-shlib-undefined" "-Wl,-z,lazy")

	add_executable(PluginLoadingTest PluginLoadingTest.cpp)
	set_default_target_properties(PluginLoadingTest)
	add_dependencies(PluginLoadingTest valid-test-plugin incompatible-test-plugin)
	target_compile_definitions(PluginLoadingTest PRIVATE
		VALID_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:valid-test-plugin>"
		INCOMPATIBLE_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:incompatible-test-plugin>"
		NON_PLUGIN_LIBRARY_FILE_PATH="$<TARGET_FILE:veyon-core>")
	target_link_libraries(PluginLoadingTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)
	add_test(NAME PluginLoadingTest COMMAND PluginLoadingTest)

endif()
