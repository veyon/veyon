include(BuildVeyonApplication)
include(WindowsBuildHelpers)

file(GLOB server_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB server_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

build_veyon_application(veyon-server ${server_SOURCES} ${server_INCLUDES})

add_windows_resource(veyon-server)
make_graphical_app(veyon-server)

target_link_libraries(veyon-server
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	)
