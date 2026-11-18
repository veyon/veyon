include(BuildVeyonApplication)

# Linux: not readable by the session user, so that a server started as the session user (Wayland)
# is non-dumpable from execve() on (T-PROC-02, 02-ipc.md §1a.4) and may get the TLS host key also
# without Yama ptrace restrictions
if(VEYON_BUILD_LINUX)
	set(VEYON_SERVER_PERMISSIONS PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)
endif()

build_veyon_application(veyon-server
	NAME Server
	${VEYON_SERVER_PERMISSIONS}
	SOURCES
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
	src/VncServer.h)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/data/io.veyon.veyon-server.desktop.in"
	"${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-server.desktop"
	@ONLY
)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/data/io.veyon.veyon-server.desktop"
	DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	COMPONENT pipewire-vnc-server
)
