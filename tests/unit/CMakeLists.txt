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
	endif()

endif()

# platform-independent tests
add_subdirectory(core)
