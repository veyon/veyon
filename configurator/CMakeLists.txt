FILE(GLOB configurator_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB configurator_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB configurator_UI ${CMAKE_CURRENT_SOURCE_DIR}/forms/*.ui)
SET(QRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/italc-configurator.qrc)

QT5_WRAP_CPP(configurator_MOC_out ${configurator_INCLUDES})
QT5_WRAP_UI(configurator_UIC_out ${configurator_UI})
QT5_ADD_RESOURCES(configurator_RCC_out ${QRC_FILE})
#QT4_TRANSLATIONS_FOR_QRC(${QRC_FILE} ${configurator_SOURCES} ${configurator_INCLUDES})

IF(ITALC_BUILD_WIN32)
	SET(WINRC "${CMAKE_CURRENT_BINARY_DIR}/italc-configurator-rc.obj")
	ADD_DEFINITIONS(-D__USE_MINGW_ANSI_STDIO=1)
	ADD_CUSTOM_COMMAND(OUTPUT ${WINRC}
						COMMAND ${WINDRES}
							-I${CMAKE_CURRENT_SOURCE_DIR}
							-o${WINRC}
							-i${CMAKE_CURRENT_BINARY_DIR}/italc-configurator.rc
						DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/italc-configurator.rc)
ENDIF(ITALC_BUILD_WIN32)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_EXECUTABLE(italc-configurator ${configurator_UIC_out} ${configurator_SOURCES} ${configurator_INCLUDES} ${configurator_MOC_out} ${configurator_RCC_out} ${WINRC})
TARGET_LINK_LIBRARIES(italc-configurator italc-core)

INSTALL(TARGETS italc-configurator RUNTIME DESTINATION bin)

IF(ITALC_BUILD_WIN32)
	SET_TARGET_PROPERTIES(italc-configurator PROPERTIES LINK_FLAGS -mconsole)
	#SET_TARGET_PROPERTIES(italc-configurator PROPERTIES LINK_FLAGS -mwindows)
ENDIF(ITALC_BUILD_WIN32)

