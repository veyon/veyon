include(BuildVeyonPlugin)

find_package(Qt5 REQUIRED COMPONENTS Core DBus)
find_package(LibVNCServer 0.9.8 REQUIRED)
find_package(PipeWire REQUIRED)

set(krfb_SOURCES
	${krfb_DIR}/krfb/events.cpp
	${krfb_DIR}/krfb/events.h
	${krfb_DIR}/krfb/framebuffer.cpp
	${krfb_DIR}/krfb/framebuffer.h
	${krfb_DIR}/events/xdp/xdpevents.cpp
	${krfb_DIR}/events/xdp/xdpevents.h
	${krfb_DIR}/framebuffers/pipewire/pw_framebuffer.cpp
	${krfb_DIR}/framebuffers/pipewire/pw_framebuffer.h
)

qt5_add_dbus_interface(
   krfb_SOURCES
   ${krfb_DIR}/dbus/xdp_dbus_screencast_interface.xml
   xdp_dbus_screencast_interface
)

qt5_add_dbus_interface(
   krfb_SOURCES
   ${krfb_DIR}/dbus/xdp_dbus_remotedesktop_interface.xml
   xdp_dbus_remotedesktop_interface
)

build_veyon_plugin(pipewire-vnc-server
	PipeWireVncServer.cpp
	PipeWireVncServer.h
	${krfb_SOURCES}
)

target_link_libraries(pipewire-vnc-server
	Qt5::DBus
	LibVNC::LibVNCServer
	PipeWire::PipeWire
)

target_include_directories(pipewire-vnc-server PRIVATE
	${krfb_DIR}/events/xdp
	${krfb_DIR}/framebuffers/pipewire
	${krfb_DIR}/krfb
)
