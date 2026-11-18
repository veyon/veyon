include(BuildVeyonApplication)

build_veyon_application(veyon-configurator
	NAME Configurator
	WINDOWS_ICON "data/veyon-configurator.ico"
	REQUIRE_ADMINISTRATOR_PRIVILEGES
	SOURCES
	src/main.cpp
	src/ConfiguratorController.cpp
	src/ConfiguratorController.h
	src/ConfiguratorDraft.h
	resources/configurator.qrc)

target_link_libraries(veyon-configurator PRIVATE Qt${QT_MAJOR_VERSION}::Quick Qt${QT_MAJOR_VERSION}::Qml)


if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-configurator.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.svg)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-configurator.policy DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions)
endif()

if(WITH_TESTS)
	add_executable(ConfiguratorQuickTest
		tests/ConfiguratorQuickTest.cpp
		src/ConfiguratorController.cpp
		src/ConfiguratorController.h
		resources/configurator.qrc)
	target_include_directories(ConfiguratorQuickTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_link_libraries(ConfiguratorQuickTest PRIVATE veyon-core Qt${QT_MAJOR_VERSION}::Test Qt${QT_MAJOR_VERSION}::Quick Qt${QT_MAJOR_VERSION}::Qml)
	set_default_target_properties(ConfiguratorQuickTest)
	add_test(NAME ConfiguratorQuickTest COMMAND ConfiguratorQuickTest)
	set_tests_properties(ConfiguratorQuickTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software" TIMEOUT 90)
endif()
