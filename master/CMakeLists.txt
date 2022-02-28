include(BuildVeyonApplication)
include(WindowsBuildHelpers)

set(kitemmodels_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/kitemmodels/src/core)
set(kitemmodels_SOURCES
	${kitemmodels_SOURCE_DIR}/kextracolumnsproxymodel.cpp
	${kitemmodels_SOURCE_DIR}/kextracolumnsproxymodel.h
	)

file(GLOB master_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB master_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui)

set(master_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/master.qrc)
if(VEYON_DEBUG)
	set(master_RESOURCES ${master_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/resources/examples.qrc)
endif()

# drop once kitemmodels compile with QT_NO_KEYWORDS and the remove_definitions/set_source_files_properties above can be removed
set(WITH_PCH OFF)
remove_definitions(-DQT_NO_KEYWORDS)
set_source_files_properties(${master_SOURCES} PROPERTIES COMPILE_DEFINITIONS QT_NO_KEYWORDS)

include_directories(${kitemmodels_SOURCE_DIR})
build_veyon_application(veyon-master ${master_SOURCES} ${master_INCLUDES} ${master_RESOURCES} ${kitemmodels_SOURCES})

add_windows_resource(veyon-master)
make_graphical_app(veyon-master)

if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
endif()

