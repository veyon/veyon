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
