include(BuildVeyonApplication)
include(WindowsBuildHelpers)

find_package(KF5ItemModels REQUIRED)

file(GLOB master_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)

set(master_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/master.qrc)
if(VEYON_DEBUG)
	set(master_RESOURCES ${master_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/resources/examples.qrc)
endif()
set(master_RESOURCES ${master_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/qml/qml.qrc)

build_veyon_application(veyon-master ${master_SOURCES} ${master_RESOURCES})

target_link_libraries(veyon-master PRIVATE KF5::ItemModels)

add_windows_resource(veyon-master)
make_graphical_app(veyon-master)

if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
endif()
