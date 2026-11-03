option(WITH_KWIN_GREETER "Build native KWin/PipeWire greeter capture" ON)
if(NOT WITH_KWIN_GREETER)
	return()
endif()

include(BuildVeyonPlugin)
find_package(PipeWire REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client>=1.18)
pkg_check_modules(XkbCommon REQUIRED IMPORTED_TARGET xkbcommon>=1.0)
find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner REQUIRED)
find_program(PIPEWIRE_EXECUTABLE pipewire)
if(NOT PIPEWIRE_EXECUTABLE)
	# The daemon is a runtime dependency and need not be on the build host.
	set(PIPEWIRE_EXECUTABLE "/usr/bin/pipewire")
endif()

set(PROTOCOL_SOURCES)
foreach(PROTOCOL kwin-screencast xdg-output fake-input keystate)
	if(PROTOCOL STREQUAL "kwin-screencast")
		set(PROTOCOL_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/zkde-screencast-unstable-v1.xml")
	elseif(PROTOCOL STREQUAL "xdg-output")
		set(PROTOCOL_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/xdg-output-unstable-v1.xml")
	elseif(PROTOCOL STREQUAL "fake-input")
		set(PROTOCOL_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/fake-input.xml")
	else()
		set(PROTOCOL_XML "${CMAKE_CURRENT_SOURCE_DIR}/protocols/keystate.xml")
	endif()
	foreach(MODE client-header server-header private-code)
		if(MODE STREQUAL "private-code")
			set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/wayland-${PROTOCOL}.c")
		elseif(MODE STREQUAL "client-header")
			set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/wayland-${PROTOCOL}-client.h")
		else()
			set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/wayland-${PROTOCOL}-server.h")
		endif()
		add_custom_command(OUTPUT "${OUTPUT_FILE}"
			COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" ${MODE} "${PROTOCOL_XML}" "${OUTPUT_FILE}"
			DEPENDS "${PROTOCOL_XML}" VERBATIM)
		list(APPEND PROTOCOL_SOURCES "${OUTPUT_FILE}")
	endforeach()
endforeach()

add_library(kwin-greeter-capture STATIC
	KWinScreencast.cpp KWinScreencast.h
	GreeterPipeWire.cpp GreeterPipeWire.h
	KWinInput.cpp KWinInput.h KWinKeyboard.cpp KWinKeyboard.h
	${PROTOCOL_SOURCES}
)
set_default_target_properties(kwin-greeter-capture)
set_target_properties(kwin-greeter-capture PROPERTIES POSITION_INDEPENDENT_CODE ON UNITY_BUILD OFF)
target_include_directories(kwin-greeter-capture PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(kwin-greeter-capture PUBLIC Qt${QT_MAJOR_VERSION}::Core PkgConfig::WaylandClient PkgConfig::XkbCommon)
target_compile_definitions(kwin-greeter-capture PRIVATE
	VEYON_PIPEWIRE_EXECUTABLE="${PIPEWIRE_EXECUTABLE}"
	VEYON_GREETER_PIPEWIRE_CONFIG="${CMAKE_INSTALL_FULL_DATADIR}/veyon/greeter-pipewire.conf"
)

build_veyon_plugin(kwin-greeter-vnc-server
	NAME KWinGreeterVncServer
	SOURCES
	KWinGreeterVncServer.cpp KWinGreeterVncServer.h
	../pipewire/PipeWireFramebuffer.cpp ../pipewire/PipeWireFramebuffer.h ../pipewire/PipeWireFrame.h
)
set_target_properties(kwin-greeter-vnc-server PROPERTIES UNITY_BUILD OFF)
target_include_directories(kwin-greeter-vnc-server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../pipewire")
target_link_libraries(kwin-greeter-vnc-server PRIVATE kwin-greeter-capture PipeWire::PipeWire)
target_compile_options(kwin-greeter-vnc-server PRIVATE -Wno-parentheses)
if(WITH_BUNDLED_LIBVNC)
	target_link_libraries(kwin-greeter-vnc-server PRIVATE vncserver)
else()
	target_link_libraries(kwin-greeter-vnc-server PRIVATE LibVNC::LibVNCServer)
endif()
install(FILES greeter-pipewire.conf DESTINATION "${CMAKE_INSTALL_DATADIR}/veyon" COMPONENT kwin-greeter-vnc-server)
