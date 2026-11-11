include(BuildVeyonApplication)

build_veyon_application(veyon-configurator
	NAME Configurator
	WINDOWS_ICON "data/veyon-configurator.ico"
	REQUIRE_ADMINISTRATOR_PRIVILEGES
	SOURCES
	src/AccessControlPage.cpp
	src/AccessControlPage.h
	src/AccessControlPage.ui
	src/AccessControlRuleEditDialog.cpp
	src/AccessControlRuleEditDialog.h
	src/AccessControlRuleEditDialog.ui
	src/AccessControlRuleListModel.cpp
	src/AccessControlRuleListModel.h
	src/AccessControlRulesTestDialog.cpp
	src/AccessControlRulesTestDialog.h
	src/AccessControlRulesTestDialog.ui
	src/GeneralConfigurationPage.cpp
	src/GeneralConfigurationPage.h
	src/GeneralConfigurationPage.ui
	src/main.cpp
	src/MainWindow.cpp
	src/MainWindow.h
	src/MainWindow.ui
	src/MasterConfigurationPage.cpp
	src/MasterConfigurationPage.h
	src/MasterConfigurationPage.ui
	src/ServiceConfigurationPage.cpp
	src/ServiceConfigurationPage.h
	src/ServiceConfigurationPage.ui
	resources/configurator.qrc)

if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-configurator.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.svg)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-configurator.policy DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions)
endif()

if(APPLE)
	set(_veyon_macos_helpers
		veyon-auth-helper
		veyon-cli
	)
	foreach(_helper ${_veyon_macos_helpers})
		if(TARGET ${_helper})
			add_dependencies(veyon-configurator ${_helper})
			if(_helper STREQUAL "veyon-auth-helper")
				set(_helper_mode 4755)
			else()
				set(_helper_mode 755)
			endif()
			add_custom_command(TARGET veyon-configurator POST_BUILD
				COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${_helper}>" "$<TARGET_FILE_DIR:veyon-configurator>/${_helper}"
				COMMAND /bin/chmod ${_helper_mode} "$<TARGET_FILE_DIR:veyon-configurator>/${_helper}"
				COMMENT "Embedding ${_helper} into veyon-configurator bundle"
				VERBATIM
			)
		endif()
	endforeach()
	unset(_helper_mode)
	unset(_veyon_macos_helpers)
endif()
