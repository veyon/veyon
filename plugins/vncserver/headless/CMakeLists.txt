include(BuildVeyonPlugin)

get_property(HAVE_LIBVNCCLIENT GLOBAL PROPERTY HAVE_LIBVNCCLIENT)
if(HAVE_LIBVNCCLIENT OR VEYON_USE_BUILTIN_LIBVNC)
	find_package(LibVNCServer 0.9.8)
endif()

if(NOT LibVNCServer_FOUND AND VEYON_USE_BUILTIN_LIBVNC)
	# When VEYON_USE_BUILTIN_LIBVNC is ON, core uses bundled libvncclient
	# so HAVE_LIBVNCCLIENT is false and LibVNCServer won't be found.
	# headless-vnc-server does not have a bundled fallback, so it won't
	# build when LibVNCServer is not available as a system package.
endif()

if(LibVNCServer_FOUND)

	build_veyon_plugin(headless-vnc-server
		NAME HeadlessVncServer
		SOURCES
		HeadlessVncServer.cpp
		HeadlessVncServer.h
		HeadlessVncConfiguration.h
		)

	target_link_libraries(headless-vnc-server PRIVATE LibVNC::LibVNCServer)

	target_compile_options(headless-vnc-server PRIVATE -Wno-parentheses)

endif()
