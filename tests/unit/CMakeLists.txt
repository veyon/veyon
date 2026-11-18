if(WITH_TESTS)

	#
	# Platform-independent unit tests (not guarded by VEYON_BUILD_LINUX)
	#
	add_executable(ConnectionLimiterTest
		core/ConnectionLimiterTest.cpp
	)
	set_default_target_properties(ConnectionLimiterTest)

	target_link_libraries(ConnectionLimiterTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Network
		veyon-core
	)

	add_test(NAME ConnectionLimiterTest COMMAND ConnectionLimiterTest)

	#
	# TextMessageSanitizerTest: HTML allowlist and link scheme checks (QtCore only)
	#
	add_executable(TextMessageSanitizerTest
		core/TextMessageSanitizerTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/textmessage/TextMessageSanitizer.cpp
	)
	set_default_target_properties(TextMessageSanitizerTest)

	target_include_directories(TextMessageSanitizerTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/textmessage)

	target_link_libraries(TextMessageSanitizerTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
	)

	add_test(NAME TextMessageSanitizerTest COMMAND TextMessageSanitizerTest)

endif()

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
	# LinuxSessionControlTest: logout/reboot/power-off requests to logind, ConsoleKit and the desktop
	# session managers with injected D-Bus calls, plus the real session bus calls against fake services.
	# The latter only run on a private session bus (dbus-run-session), never on the desktop's bus.
	#
	if(WITH_QT6)
		find_package(Qt6 COMPONENTS DBus REQUIRED)
	else()
		find_package(Qt5DBus REQUIRED)
	endif()

	add_executable(LinuxSessionControlTest
		LinuxSessionControlTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxSessionControl.cpp
	)
	set_default_target_properties(LinuxSessionControlTest)

	target_include_directories(LinuxSessionControlTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux)

	target_link_libraries(LinuxSessionControlTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::DBus
		veyon-core
	)

	find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session)
	if(DBUS_RUN_SESSION_EXECUTABLE)
		add_test(NAME LinuxSessionControlTest
			COMMAND ${DBUS_RUN_SESSION_EXECUTABLE} -- $<TARGET_FILE:LinuxSessionControlTest>)
		set_tests_properties(LinuxSessionControlTest PROPERTIES
			ENVIRONMENT VEYON_TEST_PRIVATE_SESSION_BUS=1
			TIMEOUT 60)
	else()
		add_test(NAME LinuxSessionControlTest COMMAND LinuxSessionControlTest)
	endif()

	#
	# PipeWireVncServerTest: portal session token/path generation
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

	# IpcServerTest: local IPC endpoints with real Unix domain sockets (peer credentials,
	# socket/directory modes, one connection per endpoint)
	add_executable(IpcServerTest
		IpcServerTest.cpp
	)
	set_default_target_properties(IpcServerTest)

	target_link_libraries(IpcServerTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Network
		veyon-core
	)

	add_test(NAME IpcServerTest COMMAND IpcServerTest)

	# TlsHostKeyIpcTest: GetTlsHostKey from the service's SecretStore and certificate file to the
	# session server's TLS listener through a real endpoint (02-ipc.md §6, 05-tls.md §2)
	add_executable(TlsHostKeyIpcTest
		TlsHostKeyIpcTest.cpp
		core/TlsTestHostCredentials.h
		core/PkiTestCertificates.h
		${CMAKE_SOURCE_DIR}/plugins/platform/common/AuthenticationLimiter.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/IpcAuthenticationRequest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/common/ServiceIpcCommandHandler.cpp
	)
	set_default_target_properties(TlsHostKeyIpcTest)

	target_include_directories(TlsHostKeyIpcTest PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/core
		${CMAKE_SOURCE_DIR}/plugins/platform/common
	)
	target_compile_definitions(TlsHostKeyIpcTest PRIVATE OPENSSL_API_COMPAT=10101)

	target_link_libraries(TlsHostKeyIpcTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Network
		veyon-core
		OpenSSL::Crypto
	)

	add_test(NAME TlsHostKeyIpcTest COMMAND TlsHostKeyIpcTest)

	# IpcWorkerChannelTest: server ⇄ worker channel with a real child process (02-ipc.md §6a)
	add_executable(IpcWorkerChannelTest
		IpcWorkerChannelTest.cpp
	)
	set_default_target_properties(IpcWorkerChannelTest)

	target_link_libraries(IpcWorkerChannelTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Network
		veyon-core
	)

	add_test(NAME IpcWorkerChannelTest COMMAND IpcWorkerChannelTest)

	# ProcessHardeningTest: non-dumpable server/worker processes (02-ipc.md §1a, §8.12)
	add_executable(ProcessHardeningTest
		ProcessHardeningTest.cpp
	)
	set_default_target_properties(ProcessHardeningTest)

	target_link_libraries(ProcessHardeningTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME ProcessHardeningTest COMMAND ProcessHardeningTest)

	# LinuxSecretFunctionsTest: SecretStore backend of the Linux platform plugin (03-secretstore-config.md §2, §6)
	add_executable(LinuxSecretFunctionsTest
		LinuxSecretFunctionsTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxSecretFunctions.cpp
	)
	set_default_target_properties(LinuxSecretFunctionsTest)

	target_include_directories(LinuxSecretFunctionsTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux)

	target_link_libraries(LinuxSecretFunctionsTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)

	add_test(NAME LinuxSecretFunctionsTest COMMAND LinuxSecretFunctionsTest)

	set(PLATFORM_LINUX_SOURCE_DIR ${CMAKE_SOURCE_DIR}/plugins/platform/linux)
	set(PLATFORM_COMMON_SOURCE_DIR ${CMAKE_SOURCE_DIR}/plugins/platform/common)

	if(WITH_QT6)
		find_package(Qt6 COMPONENTS DBus REQUIRED)
	else()
		find_package(Qt5DBus REQUIRED)
	endif()

	# LinuxServiceAuthenticationTest: Logon authentication of the session server through the
	# service IPC with the real platform plugin and ServiceDataManager (02-ipc.md §6b)
	add_executable(LinuxServiceAuthenticationTest
		LinuxServiceAuthenticationTest.cpp
		${PLATFORM_COMMON_SOURCE_DIR}/AuthenticationLimiter.cpp
		${PLATFORM_COMMON_SOURCE_DIR}/IpcAuthenticationRequest.cpp
		${PLATFORM_COMMON_SOURCE_DIR}/ServiceDataManager.cpp
		${PLATFORM_COMMON_SOURCE_DIR}/ServiceIpcCommandHandler.cpp
		${PLATFORM_LINUX_SOURCE_DIR}/LinuxAuthHelperClient.cpp
	)
	set_default_target_properties(LinuxServiceAuthenticationTest)
	add_dependencies(LinuxServiceAuthenticationTest linux-platform)
	target_include_directories(LinuxServiceAuthenticationTest PRIVATE
		${PLATFORM_LINUX_SOURCE_DIR}
		${PLATFORM_COMMON_SOURCE_DIR}
	)
	target_link_libraries(LinuxServiceAuthenticationTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::DBus
		Qt${QT_MAJOR_VERSION}::Network
		veyon-core
	)
	add_test(NAME LinuxServiceAuthenticationTest COMMAND LinuxServiceAuthenticationTest)

	# SecretPropertyPluginsTest: the secret property rows the real plugins of the build tree declare
	# match their properties (03-secretstore-config.md §3a)
	add_executable(SecretPropertyPluginsTest SecretPropertyPluginsTest.cpp)
	set_default_target_properties(SecretPropertyPluginsTest)
	add_dependencies(SecretPropertyPluginsTest linux-platform external-vnc-server)
	if(TARGET ldap)
		add_dependencies(SecretPropertyPluginsTest ldap)
		target_compile_definitions(SecretPropertyPluginsTest PRIVATE VEYON_TEST_WITH_LDAP_PLUGIN)
	endif()
	target_link_libraries(SecretPropertyPluginsTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)
	add_test(NAME SecretPropertyPluginsTest COMMAND SecretPropertyPluginsTest)

	# LinuxLogFileDirectoryTest: log file directories of root and unprivileged processes (WP23)
	add_executable(LinuxLogFileDirectoryTest
		LinuxLogFileDirectoryTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxFilesystemFunctions.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxLogFileDirectory.cpp
	)
	set_default_target_properties(LinuxLogFileDirectoryTest)
	target_include_directories(LinuxLogFileDirectoryTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux)
	target_link_libraries(LinuxLogFileDirectoryTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)
	add_test(NAME LinuxLogFileDirectoryTest COMMAND LinuxLogFileDirectoryTest)

	# LinuxUserProcessTest: programs started by LinuxCoreFunctions::runProgramAsUser() outlive the
	# starting process without being killed by SIGPIPE
	add_executable(LinuxUserProcessTest
		LinuxUserProcessTest.cpp
	)
	set_default_target_properties(LinuxUserProcessTest)
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

	# LinuxUserFunctionsTest: user environment (HOME etc.) of servers from the password database
	add_executable(LinuxUserFunctionsTest
		LinuxUserFunctionsTest.cpp
	)
	set_default_target_properties(LinuxUserFunctionsTest)
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

	# DesktopServicesFeatureTest: website URLs handled by the worker after serialization
	if(TARGET desktopservices)
		add_executable(DesktopServicesFeatureTest
			DesktopServicesFeatureTest.cpp
		)
		set_default_target_properties(DesktopServicesFeatureTest)
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

	# LoggerTest: Logger with the Linux platform plugin in child processes (unprivileged user, root
	# in a private mount namespace with a tmpfs on /var/log; WP23)
	add_executable(LoggerTest
		LoggerTest.cpp
	)
	set_default_target_properties(LoggerTest)
	add_dependencies(LoggerTest linux-platform)
	target_link_libraries(LoggerTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)
	add_test(NAME LoggerTest COMMAND LoggerTest)

	# PluginLoadingTest: plugins with unresolved symbols (e.g. built for an older Veyon version) are
	# refused at load time and logged. Linux only: on Windows a DLL with an undefined symbol does not link.
	# Plugins built for another Veyon version or without build version are refused before instantiation.
	include(BuildVeyonPlugin)

	add_library(valid-test-plugin MODULE plugin-loading/ValidTestPlugin.cpp)
	target_compile_definitions(valid-test-plugin PRIVATE TEST_PLUGIN_NAME="valid")
	target_link_libraries(valid-test-plugin PRIVATE Qt${QT_MAJOR_VERSION}::Core)
	add_veyon_plugin_build_version(valid-test-plugin)

	# no set_default_target_properties(): it links with -no-undefined
	add_library(incompatible-test-plugin MODULE plugin-loading/IncompatibleTestPlugin.cpp)
	target_link_libraries(incompatible-test-plugin PRIVATE Qt${QT_MAJOR_VERSION}::Core)
	target_link_options(incompatible-test-plugin PRIVATE "-Wl,--allow-shlib-undefined" "-Wl,-z,lazy")
	add_veyon_plugin_build_version(incompatible-test-plugin)

	# like a plugin built for the previous or next patch release
	math(EXPR OTHER_VERSION_PATCH "${VERSION_PATCH} + 1")
	add_library(other-version-test-plugin MODULE plugin-loading/ValidTestPlugin.cpp)
	target_compile_definitions(other-version-test-plugin PRIVATE TEST_PLUGIN_NAME="other-version")
	target_link_libraries(other-version-test-plugin PRIVATE Qt${QT_MAJOR_VERSION}::Core)
	add_veyon_plugin_build_version(other-version-test-plugin
		VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${OTHER_VERSION_PATCH}")

	# like a plugin built for a Veyon version without build version check
	add_library(unversioned-test-plugin MODULE plugin-loading/ValidTestPlugin.cpp)
	target_compile_definitions(unversioned-test-plugin PRIVATE TEST_PLUGIN_NAME="unversioned")
	target_link_libraries(unversioned-test-plugin PRIVATE Qt${QT_MAJOR_VERSION}::Core)

	add_executable(PluginLoadingTest PluginLoadingTest.cpp)
	set_default_target_properties(PluginLoadingTest)
	add_dependencies(PluginLoadingTest
		valid-test-plugin incompatible-test-plugin other-version-test-plugin unversioned-test-plugin)
	target_compile_definitions(PluginLoadingTest PRIVATE
		VALID_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:valid-test-plugin>"
		INCOMPATIBLE_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:incompatible-test-plugin>"
		OTHER_VERSION_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:other-version-test-plugin>"
		OTHER_VERSION_TEST_PLUGIN_VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${OTHER_VERSION_PATCH}"
		UNVERSIONED_TEST_PLUGIN_FILE_PATH="$<TARGET_FILE:unversioned-test-plugin>"
		NON_PLUGIN_LIBRARY_FILE_PATH="$<TARGET_FILE:veyon-core>")
	target_link_libraries(PluginLoadingTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test
		Qt${QT_MAJOR_VERSION}::Core
		veyon-core
	)
	add_test(NAME PluginLoadingTest COMMAND PluginLoadingTest)

	# veyon-capture-helper (02-ipc.md §1a.5): stdin credentials line and supervision, and the real
	# helper started from a non-dumpable process (loopback only, VNC password, dumpable helper)
	if(TARGET veyon-capture-helper)
		set(CAPTURE_HELPER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/plugins/vncserver/pipewire)

		add_executable(CaptureHelperCredentialsTest
			CaptureHelperCredentialsTest.cpp
			${CAPTURE_HELPER_SOURCE_DIR}/CaptureHelperCredentials.cpp
		)
		set_default_target_properties(CaptureHelperCredentialsTest)
		target_include_directories(CaptureHelperCredentialsTest PRIVATE ${CAPTURE_HELPER_SOURCE_DIR})
		target_link_libraries(CaptureHelperCredentialsTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
			veyon-core
		)
		add_test(NAME CaptureHelperCredentialsTest COMMAND CaptureHelperCredentialsTest)

		add_executable(CaptureHelperCoreTest CaptureHelperCoreTest.cpp)
		set_default_target_properties(CaptureHelperCoreTest)
		add_dependencies(CaptureHelperCoreTest linux-platform)
		target_link_libraries(CaptureHelperCoreTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
			veyon-core
		)
		add_test(NAME CaptureHelperCoreTest
			COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run-with-private-log-directory.sh $<TARGET_FILE:CaptureHelperCoreTest>)
		set_tests_properties(CaptureHelperCoreTest PROPERTIES SKIP_RETURN_CODE 77)

		add_executable(CaptureHelperProcessTest
			CaptureHelperProcessTest.cpp
			${CAPTURE_HELPER_SOURCE_DIR}/CaptureHelperCredentials.cpp
			${CAPTURE_HELPER_SOURCE_DIR}/CaptureHelperProcess.cpp
		)
		set_default_target_properties(CaptureHelperProcessTest)
		target_include_directories(CaptureHelperProcessTest PRIVATE ${CAPTURE_HELPER_SOURCE_DIR})
		target_compile_definitions(CaptureHelperProcessTest PRIVATE
			CAPTURE_HELPER_FILE_PATH="$<TARGET_FILE:veyon-capture-helper>")
		add_dependencies(CaptureHelperProcessTest veyon-capture-helper pipewire-vnc-server)
		target_link_libraries(CaptureHelperProcessTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
			Qt${QT_MAJOR_VERSION}::Network
			veyon-core
		)
		add_test(NAME CaptureHelperProcessTest COMMAND CaptureHelperProcessTest)

		# PipeWire frame validation and copy of the helper; SPA headers only, no PipeWire runtime
		add_executable(PipeWireFrameTest
			PipeWireFrameTest.cpp
			${CAPTURE_HELPER_SOURCE_DIR}/PipeWireFrame.cpp
		)
		set_default_target_properties(PipeWireFrameTest)
		target_include_directories(PipeWireFrameTest PRIVATE ${CAPTURE_HELPER_SOURCE_DIR} ${Spa_INCLUDE_DIR})
		target_link_libraries(PipeWireFrameTest PRIVATE
			Qt${QT_MAJOR_VERSION}::Test
			Qt${QT_MAJOR_VERSION}::Core
		)
		add_test(NAME PipeWireFrameTest COMMAND PipeWireFrameTest)

		# input order of PortalSession and PortalInputDispatcher against fake portal services; only on
		# a private session bus (dbus-run-session), never on the desktop's bus
		if(DBUS_RUN_SESSION_EXECUTABLE)
			add_executable(PortalInputDispatcherTest
				PortalInputDispatcherTest.cpp
				${CAPTURE_HELPER_SOURCE_DIR}/PortalInputDispatcher.cpp
			)
			set_default_target_properties(PortalInputDispatcherTest)
			target_include_directories(PortalInputDispatcherTest PRIVATE ${CAPTURE_HELPER_SOURCE_DIR})
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
				${CAPTURE_HELPER_SOURCE_DIR}/dbus/org.freedesktop.portal.RemoteDesktop.xml
				OrgFreedesktopPortalRemoteDesktop
			)
			qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
				${CAPTURE_HELPER_SOURCE_DIR}/dbus/org.freedesktop.portal.ScreenCast.xml
				OrgFreedesktopPortalScreenCast
			)
			qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
				${CAPTURE_HELPER_SOURCE_DIR}/dbus/org.freedesktop.portal.Request.xml
				OrgFreedesktopPortalRequest
			)
			qt_add_dbus_interface(PORTAL_DBUS_INTERFACE_SOURCES
				${CAPTURE_HELPER_SOURCE_DIR}/dbus/org.freedesktop.portal.Session.xml
				OrgFreedesktopPortalSession
			)

			add_executable(PortalSessionTest
				PortalSessionTest.cpp
				${CAPTURE_HELPER_SOURCE_DIR}/PortalInputDispatcher.cpp
				${CAPTURE_HELPER_SOURCE_DIR}/PortalSession.cpp
				${PORTAL_DBUS_INTERFACE_SOURCES}
			)
			set_default_target_properties(PortalSessionTest)
			target_include_directories(PortalSessionTest PRIVATE
				${CAPTURE_HELPER_SOURCE_DIR}
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
	endif()

endif()

# platform-independent tests
add_subdirectory(core)
