INCLUDE(WindowsBuildHelpers)

FILE(GLOB configurator_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB configurator_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
SET(configurator_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/veyon-configurator.qrc)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_EXECUTABLE(veyon-configurator ${configurator_SOURCES} ${configurator_INCLUDES} ${configurator_RESOURCES})
TARGET_LINK_LIBRARIES(veyon-configurator veyon-core)

ADD_WINDOWS_RESOURCE(veyon-configurator)
MAKE_GRAPHICAL_APP(veyon-configurator)

SET_TARGET_PROPERTIES(veyon-configurator PROPERTIES AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

INSTALL(TARGETS veyon-configurator RUNTIME DESTINATION bin)

IF(VEYON_BUILD_LINUX)
	XDG_INSTALL(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-configurator.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.svg)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-configurator.policy DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions)
ENDIF()

COTIRE_VEYON(veyon-configurator)
