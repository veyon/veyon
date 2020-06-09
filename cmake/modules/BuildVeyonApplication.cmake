# BuildVeyonApplication.cmake - Copyright (c) 2019-2020 Tobias Junghans
#
# description: build Veyon application
# usage: build_veyon_application(<NAME> <SOURCES>)

MACRO(build_veyon_application APPLICATION_NAME)
	IF(VEYON_BUILD_ANDROID)
		ADD_LIBRARY(${APPLICATION_NAME} SHARED ${ARGN})
	ELSE()
		ADD_EXECUTABLE(${APPLICATION_NAME} ${ARGN})
		INSTALL(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION bin)
	ENDIF()
	set_default_target_properties(${APPLICATION_NAME})
ENDMACRO()

