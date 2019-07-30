INCLUDE(BuildApplication)
INCLUDE(WindowsBuildHelpers)

SET(kitemmodels_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/kitemmodels/src)
SET(kitemmodels_SOURCES
	${kitemmodels_SOURCE_DIR}/kextracolumnsproxymodel.cpp
	${kitemmodels_SOURCE_DIR}/krecursivefilterproxymodel.cpp
	${kitemmodels_SOURCE_DIR}/kextracolumnsproxymodel.h
	${kitemmodels_SOURCE_DIR}/krecursivefilterproxymodel.h
)

FILE(GLOB master_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB master_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui)

SET(master_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/master.qrc)
IF(VEYON_DEBUG)
SET(master_RESOURCES ${master_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/resources/examples.qrc)
ENDIF()

BUILD_APPLICATION(veyon-master ${master_SOURCES} ${master_INCLUDES} ${master_RESOURCES} ${kitemmodels_SOURCES})
TARGET_INCLUDE_DIRECTORIES(veyon-master PRIVATE ${kitemmodels_SOURCE_DIR})

ADD_WINDOWS_RESOURCE(veyon-master)
MAKE_GRAPHICAL_APP(veyon-master)

IF(VEYON_BUILD_LINUX)
	XDG_INSTALL(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
ENDIF()

COTIRE_VEYON(veyon-master)
