include(BuildVeyonPlugin)

build_veyon_plugin(headless-vnc-server
	NAME HeadlessVncServer
	SOURCES
	HeadlessVncServer.cpp
	HeadlessVncServer.h
	HeadlessVncConfiguration.h
)

if(WITH_BUNDLED_LIBVNC)
	target_link_libraries(headless-vnc-server PRIVATE vncserver)
else()
	target_link_libraries(headless-vnc-server PRIVATE LibVNC::LibVNCServer)
endif()

target_compile_options(headless-vnc-server PRIVATE -Wno-parentheses)
