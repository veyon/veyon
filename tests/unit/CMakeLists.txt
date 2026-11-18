if(WITH_TESTS AND VEYON_BUILD_LINUX)

	find_package(X11 REQUIRED)

	#
	# LinuxKeyboardMappingTest: tests keysym → linux keycode mapping tables
	# and UTF-8 → linux keycode conversion.
	#
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(keyboard_fakekey QUIET IMPORTED_TARGET libfakekey)
	add_executable(LinuxKeyboardMappingTest
		LinuxKeyboardMappingTest.cpp
		${CMAKE_SOURCE_DIR}/plugins/platform/linux/LinuxKeyboardInput.cpp
	)

	target_include_directories(LinuxKeyboardMappingTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux
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

	if(keyboard_fakekey_FOUND)
		target_link_libraries(LinuxKeyboardMappingTest PRIVATE PkgConfig::keyboard_fakekey)
	else()
		target_sources(LinuxKeyboardMappingTest PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/libfakekey/src/libfakekey.c)
		target_include_directories(LinuxKeyboardMappingTest PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/libfakekey)
	endif()

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

endif()

if(WITH_TESTS)
	add_executable(ConfigurationAspectTest ConfigurationAspectTest.cpp)
	target_link_libraries(ConfigurationAspectTest PRIVATE
		Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Core veyon-core)
	add_test(NAME ConfigurationAspectTest COMMAND ConfigurationAspectTest)
endif()

if(WITH_TESTS)
	add_executable(ConfiguratorDraftTest ConfiguratorDraftTest.cpp)
	target_include_directories(ConfiguratorDraftTest PRIVATE ${CMAKE_SOURCE_DIR}/configurator/src)
	target_link_libraries(ConfiguratorDraftTest PRIVATE Qt${QT_MAJOR_VERSION}::Test veyon-core)
	add_test(NAME ConfiguratorDraftTest COMMAND ConfiguratorDraftTest)
endif()

if(WITH_TESTS)
	add_executable(DesktopServicesFeatureTest DesktopServicesFeatureTest.cpp)
	target_include_directories(DesktopServicesFeatureTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/desktopservices)
	target_link_libraries(DesktopServicesFeatureTest PRIVATE veyon-core desktopservices Qt${QT_MAJOR_VERSION}::Test)
	set_default_target_properties(DesktopServicesFeatureTest)
	add_test(NAME DesktopServicesFeatureTest COMMAND DesktopServicesFeatureTest)
	set_tests_properties(DesktopServicesFeatureTest PROPERTIES
		ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
		TIMEOUT 30)
endif()

if(WITH_TESTS AND VEYON_BUILD_LINUX)
	add_executable(LinuxUserProcessTest LinuxUserProcessTest.cpp)
	target_include_directories(LinuxUserProcessTest PRIVATE
		${CMAKE_SOURCE_DIR}/plugins/platform/linux
		$<TARGET_PROPERTY:linux-platform,INCLUDE_DIRECTORIES>)
	target_compile_definitions(LinuxUserProcessTest PRIVATE
		$<TARGET_PROPERTY:linux-platform,COMPILE_DEFINITIONS>)
	target_link_libraries(LinuxUserProcessTest PRIVATE
		veyon-core linux-platform Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::DBus)
	set_default_target_properties(LinuxUserProcessTest)
	add_test(NAME LinuxUserProcessTest COMMAND LinuxUserProcessTest)
	set_tests_properties(LinuxUserProcessTest PROPERTIES TIMEOUT 30)
endif()
