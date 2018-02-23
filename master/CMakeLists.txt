INCLUDE(WindowsBuildHelpers)

FILE(GLOB master_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB master_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB master_UI ${CMAKE_CURRENT_SOURCE_DIR}/forms/*.ui)
SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/master.qrc)
QT5_WRAP_CPP(master_MOC_out ${master_INCLUDES})
QT5_WRAP_UI(master_UIC_out ${master_UI})
QT5_ADD_RESOURCES(master_RCC_out ${QRC_FILE})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_EXECUTABLE(veyon-master ${master_UIC_out} ${master_SOURCES} ${master_INCLUDES} ${master_MOC_out} ${master_RCC_out})
TARGET_LINK_LIBRARIES(veyon-master veyon-core)

ADD_WINDOWS_RESOURCE(veyon-master)
MAKE_GRAPHICAL_APP(veyon-master)

INSTALL(TARGETS veyon-master RUNTIME DESTINATION bin)

IF(VEYON_BUILD_LINUX)
	XDG_INSTALL(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
ENDIF()

COTIRE_VEYON(veyon-master)
