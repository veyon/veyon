include(BuildVeyonPlugin)

find_package(LibVNCServer 0.9.8)

if(LibVNCServer_FOUND)

build_veyon_plugin(headless-vnc-server
	HeadlessVncServer.cpp
	HeadlessVncServer.h
	HeadlessVncConfiguration.h
)

target_link_libraries(headless-vnc-server LibVNC::LibVNCServer)

endif()

