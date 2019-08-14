INCLUDE(BuildApplication)
INCLUDE(WindowsBuildHelpers)

FILE(GLOB server_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB server_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

BUILD_APPLICATION(veyon-server ${server_SOURCES} ${server_INCLUDES})

ADD_WINDOWS_RESOURCE(veyon-server)
MAKE_GRAPHICAL_APP(veyon-server)

TARGET_LINK_LIBRARIES(veyon-server
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
	)

COTIRE_VEYON(veyon-server)

IF(VEYON_BUILD_ANDROID)
	SET(CMAKE_ANDROID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
	androiddeployqt("veyon-server" "${ANDROID_ADDITIONAL_FIND_ROOT_PATH};${CMAKE_BINARY_DIR}/core")
	SET_TARGET_PROPERTIES(create-apk-veyon-server PROPERTIES ANDROID_APK_DIR "${CMAKE_ANDROID_DIR}")

	add_dependencies(create-apk-veyon-server prepare-apk)
ENDIF()
