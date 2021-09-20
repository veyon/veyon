# BuildVeyonPlugin.cmake - Copyright (c) 2017-2021 Tobias Junghans
#
# description: build Veyon plugin
# usage: build_veyon_plugin(<NAME> <SOURCES>)

include(SetDefaultTargetProperties)

macro(build_veyon_plugin PLUGIN_NAME)
	add_library(${PLUGIN_NAME} MODULE ${ARGN})

	target_include_directories(${PLUGIN_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(${PLUGIN_NAME} veyon-core)
	target_compile_options(${PLUGIN_NAME} PRIVATE ${VEYON_COMPILE_OPTIONS})

	set_default_target_properties(${PLUGIN_NAME})
	set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
	set_target_properties(${PLUGIN_NAME} PROPERTIES LINK_FLAGS "-Wl,-no-undefined")
	install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION ${VEYON_INSTALL_PLUGIN_DIR})
	if(WITH_PCH)
		target_precompile_headers(${PLUGIN_NAME} REUSE_FROM veyon-pch)
	endif()
endmacro()

