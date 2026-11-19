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
