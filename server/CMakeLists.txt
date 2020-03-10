include(BuildVeyonApplication)
include(WindowsBuildHelpers)

file(GLOB server_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB server_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

build_veyon_application(veyon-server ${server_SOURCES} ${server_INCLUDES})

add_windows_resource(veyon-server)
make_graphical_app(veyon-server)

target_link_libraries(veyon-server
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
	)

cotire_veyon(veyon-server)

if(VEYON_BUILD_ANDROID)
	set(CMAKE_ANDROID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
	androiddeployqt("veyon-server" "${ANDROID_ADDITIONAL_FIND_ROOT_PATH};${CMAKE_BINARY_DIR}/core;${ANDROID_INSTALL_DIR}")
	set_target_properties(create-apk-veyon-server PROPERTIES ANDROID_APK_DIR "${CMAKE_ANDROID_DIR}")

	add_dependencies(create-apk-veyon-server prepare-apk)
endif()
