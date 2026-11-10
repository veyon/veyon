if(WITH_TESTS AND VEYON_BUILD_LINUX)
	add_executable(GreeterVncServerTest
		GreeterVncServerTest.cpp
		${CMAKE_SOURCE_DIR}/server/src/GreeterVncServer.cpp
	)
	target_include_directories(GreeterVncServerTest PRIVATE ${CMAKE_SOURCE_DIR}/server/src)
	set_default_target_properties(GreeterVncServerTest)
	target_link_libraries(GreeterVncServerTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Network veyon-core)
	add_test(NAME GreeterVncServerTest COMMAND GreeterVncServerTest)
	set_tests_properties(GreeterVncServerTest PROPERTIES TIMEOUT 20)

	find_package(X11 REQUIRED)
	add_executable(GreeterEnvironmentTest
		GreeterEnvironmentTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/GreeterEnvironment.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/SessionFileAccess.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/LoginSession.cpp
	)
	target_include_directories(GreeterEnvironmentTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux)
	set_default_target_properties(GreeterEnvironmentTest)
	target_link_libraries(GreeterEnvironmentTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::DBus Qt${QT_MAJOR_VERSION}::Network)
	add_test(NAME GreeterEnvironmentTest COMMAND GreeterEnvironmentTest)

	add_executable(SessionFileAccessTest
		SessionFileAccessTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/SessionFileAccess.cpp
	)
	target_include_directories(SessionFileAccessTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux)
	set_default_target_properties(SessionFileAccessTest)
	target_link_libraries(SessionFileAccessTest PRIVATE Qt${QT_MAJOR_VERSION}::Test)
	add_test(NAME SessionFileAccessTest COMMAND SessionFileAccessTest)

	add_executable(LightDmLoginTest
		LightDmLoginTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/LightDmAdapter.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/LoginSession.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/FdChannel.cpp
	)
	target_include_directories(LightDmLoginTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper
		${PAM_INCLUDE_DIR}
	)
	set_default_target_properties(LightDmLoginTest)
	target_link_libraries(LightDmLoginTest PRIVATE Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::DBus)
	add_test(NAME LightDmLoginTest COMMAND LightDmLoginTest)
	add_dependencies(LightDmLoginTest veyon-login-helper)
	add_executable(LightDmSessionTest
		LightDmSessionTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/ProcessIdentity.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/LightDmSession.cpp
	)
	target_include_directories(LightDmSessionTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper)
	set_default_target_properties(LightDmSessionTest)
	target_link_libraries(LightDmSessionTest PRIVATE Qt${QT_MAJOR_VERSION}::Test)
	add_test(NAME LightDmSessionTest COMMAND LightDmSessionTest)
	add_executable(LoginProcessIdentityTest
		LoginProcessIdentityTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/ProcessIdentity.cpp
	)
	target_include_directories(LoginProcessIdentityTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper)
	set_default_target_properties(LoginProcessIdentityTest)
	target_link_libraries(LoginProcessIdentityTest PRIVATE Qt${QT_MAJOR_VERSION}::Test)
	add_test(NAME LoginProcessIdentityTest COMMAND LoginProcessIdentityTest)
	find_package(Python3 COMPONENTS Interpreter QUIET)
	find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session)
	if(TARGET kwin-greeter-capture)
		pkg_check_modules(WaylandServer QUIET IMPORTED_TARGET wayland-server>=1.18)
		if(WaylandServer_FOUND)
			add_executable(KWinScreencastTest KWinScreencastTest.cpp)
			set_default_target_properties(KWinScreencastTest)
			target_link_libraries(KWinScreencastTest PRIVATE kwin-greeter-capture
				Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Network PkgConfig::WaylandServer)
			add_test(NAME KWinScreencastTest COMMAND KWinScreencastTest)
			set_tests_properties(KWinScreencastTest PROPERTIES TIMEOUT 30)
		endif()
	endif()
	if(Python3_Interpreter_FOUND AND DBUS_RUN_SESSION_EXECUTABLE)
		add_test(NAME LightDmWrapperTest COMMAND ${DBUS_RUN_SESSION_EXECUTABLE} --
			${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/lightdm_wrapper_test.py $<TARGET_FILE:veyon-login-helper>
			${CMAKE_SOURCE_DIR}/plugins/platform/linux/login-helper/veyon-lightdm-greeter.in)
		set_tests_properties(LightDmWrapperTest PROPERTIES TIMEOUT 60)
	endif()

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
	add_executable(PipeWireFrameTest PipeWireFrameTest.cpp)
	set_default_target_properties(PipeWireFrameTest)
	target_include_directories(PipeWireFrameTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/vncserver/pipewire)
	target_link_libraries(PipeWireFrameTest PRIVATE Qt${QT_MAJOR_VERSION}::Test)
	add_test(NAME PipeWireFrameTest COMMAND PipeWireFrameTest)

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

endif()
