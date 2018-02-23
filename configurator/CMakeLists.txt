INCLUDE(WindowsBuildHelpers)

FILE(GLOB configurator_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB configurator_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB configurator_UI ${CMAKE_CURRENT_SOURCE_DIR}/forms/*.ui)
SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/veyon-configurator.qrc)

QT5_WRAP_CPP(configurator_MOC_out ${configurator_INCLUDES})
QT5_WRAP_UI(configurator_UIC_out ${configurator_UI})
QT5_ADD_RESOURCES(configurator_RCC_out ${QRC_FILE})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_EXECUTABLE(veyon-configurator ${configurator_UIC_out} ${configurator_SOURCES} ${configurator_INCLUDES} ${configurator_MOC_out} ${configurator_RCC_out})
TARGET_LINK_LIBRARIES(veyon-configurator veyon-core)

ADD_WINDOWS_RESOURCE(veyon-configurator)
MAKE_GRAPHICAL_APP(veyon-configurator)

INSTALL(TARGETS veyon-configurator RUNTIME DESTINATION bin)

IF(VEYON_BUILD_LINUX)
	XDG_INSTALL(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-configurator.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.svg)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-configurator.policy DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions)
ENDIF()

COTIRE_VEYON(veyon-configurator)
