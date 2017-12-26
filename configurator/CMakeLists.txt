FILE(GLOB configurator_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB configurator_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB configurator_UI ${CMAKE_CURRENT_SOURCE_DIR}/forms/*.ui)
SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/veyon-configurator.qrc)

QT5_WRAP_CPP(configurator_MOC_out ${configurator_INCLUDES})
QT5_WRAP_UI(configurator_UIC_out ${configurator_UI})
QT5_ADD_RESOURCES(configurator_RCC_out ${QRC_FILE})

IF(VEYON_BUILD_WIN32)
	SET(WINRC "${CMAKE_CURRENT_BINARY_DIR}/veyon-configurator-rc.obj")
	ADD_DEFINITIONS(-DUNICODE -D_UNICODE)
	ADD_CUSTOM_COMMAND(OUTPUT ${WINRC}
						COMMAND ${WINDRES}
							-I${CMAKE_CURRENT_SOURCE_DIR}
							-o${WINRC}
							-i${CMAKE_CURRENT_BINARY_DIR}/veyon-configurator.rc
						DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/veyon-configurator.rc)
ENDIF(VEYON_BUILD_WIN32)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_EXECUTABLE(veyon-configurator ${configurator_UIC_out} ${configurator_SOURCES} ${configurator_INCLUDES} ${configurator_MOC_out} ${configurator_RCC_out} ${WINRC})
TARGET_LINK_LIBRARIES(veyon-configurator veyon-core)

INSTALL(TARGETS veyon-configurator RUNTIME DESTINATION bin)

IF(VEYON_BUILD_WIN32)
	SET_TARGET_PROPERTIES(veyon-configurator PROPERTIES LINK_FLAGS -mwindows)
ENDIF(VEYON_BUILD_WIN32)

IF(VEYON_BUILD_LINUX)
	XDG_INSTALL(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-configurator.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-configurator.svg)
	INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-configurator.policy DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions)
ENDIF()
