include(BuildVeyonApplication)
include(WindowsBuildHelpers)

file(GLOB configurator_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB configurator_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui)
set(configurator_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/configurator.qrc)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
build_veyon_application(veyon-configurator ${configurator_SOURCES} ${configurator_INCLUDES} ${configurator_RESOURCES})
target_link_libraries(veyon-configurator veyon-core)

add_windows_resource(veyon-configurator)
make_graphical_app(veyon-configurator)

if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-configurator.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.svg)
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-configurator.policy DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions)
endif()
