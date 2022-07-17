include(BuildVeyonPlugin)

get_property(HAVE_LIBVNCCLIENT GLOBAL PROPERTY HAVE_LIBVNCCLIENT)
if(HAVE_LIBVNCCLIENT)
	find_package(LibVNCServer 0.9.8)
endif()

if(LibVNCServer_FOUND)

	build_veyon_plugin(headless-vnc-server
		HeadlessVncServer.cpp
		HeadlessVncServer.h
		HeadlessVncConfiguration.h
		)

	target_link_libraries(headless-vnc-server PRIVATE LibVNC::LibVNCServer)

	target_compile_options(headless-vnc-server PRIVATE -Wno-parentheses)

endif()
