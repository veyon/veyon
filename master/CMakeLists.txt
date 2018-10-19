INCLUDE(WindowsBuildHelpers)

SET(kitemmodels_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/kitemmodels/src)
SET(kitemmodels_SOURCES ${kitemmodels_SOURCE_DIR}/kextracolumnsproxymodel.cpp)
SET(kitemmodels_HEADERS ${kitemmodels_SOURCE_DIR}/kextracolumnsproxymodel.h)

FILE(GLOB master_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB master_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/master.qrc)
QT5_ADD_RESOURCES(master_RCC_out ${QRC_FILE})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${kitemmodels_SOURCE_DIR})
ADD_EXECUTABLE(veyon-master ${master_SOURCES} ${master_INCLUDES} ${master_RCC_out} ${kitemmodels_SOURCES} ${kitemmodels_HEADERS})
TARGET_LINK_LIBRARIES(veyon-master veyon-core)

SET_TARGET_PROPERTIES(veyon-master PROPERTIES AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

ADD_WINDOWS_RESOURCE(veyon-master)
MAKE_GRAPHICAL_APP(veyon-master)

INSTALL(TARGETS veyon-master RUNTIME DESTINATION bin)

IF(VEYON_BUILD_LINUX)
	XDG_INSTALL(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
ENDIF()

COTIRE_VEYON(veyon-master)
