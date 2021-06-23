include(BuildVeyonApplication)
include(WindowsBuildHelpers)

build_veyon_application(veyon-server
	src/ComputerControlClient.cpp
	src/ComputerControlClient.h
	src/ComputerControlServer.cpp
	src/ComputerControlServer.h
	src/main.cpp
	src/ServerAccessControlManager.cpp
	src/ServerAccessControlManager.h
	src/ServerAuthenticationManager.cpp
	src/ServerAuthenticationManager.h
	src/VeyonServerProtocol.cpp
	src/VeyonServerProtocol.h
	src/VncProxyConnection.cpp
	src/VncProxyConnectionFactory.h
	src/VncProxyConnection.h
	src/VncProxyServer.cpp
	src/VncProxyServer.h
	src/VncServer.cpp
	src/VncServer.h
	)

add_windows_resource(veyon-server)
make_graphical_app(veyon-server)

if(VEYON_BUILD_ANDROID)
	set(CMAKE_ANDROID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
	androiddeployqt("veyon-server" "${ANDROID_ADDITIONAL_FIND_ROOT_PATH};${CMAKE_BINARY_DIR}/core;${ANDROID_INSTALL_DIR}")
	set_target_properties(create-apk-veyon-server PROPERTIES ANDROID_APK_DIR "${CMAKE_ANDROID_DIR}")

	add_dependencies(create-apk-veyon-server prepare-apk)
endif()
