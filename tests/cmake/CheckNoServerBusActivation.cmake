# fails if veyon-server can be started by D-Bus activation again (it must only be started by
# veyon-service, 02-ipc.md §2a.2) or if the capture helper claims a well-known bus name, which
# xdg-desktop-portal does not use for the app ID (02-ipc.md §1a.5)

file(GLOB serviceFiles ${VEYON_SOURCE_DIR}/server/data/*.service*)
if(serviceFiles)
	message(FATAL_ERROR "D-Bus activation files for veyon-server: ${serviceFiles}")
endif()

file(READ ${VEYON_SOURCE_DIR}/server/CMakeLists.txt content)
string(FIND "${content}" "dbus-1/services" position)
if(NOT position EQUAL -1)
	message(FATAL_ERROR "server/CMakeLists.txt still installs a D-Bus activation file")
endif()

file(READ ${VEYON_SOURCE_DIR}/plugins/vncserver/pipewire/PortalSession.cpp content)
string(FIND "${content}" "registerService" position)
if(NOT position EQUAL -1)
	message(FATAL_ERROR "PortalSession still registers a well-known bus name")
endif()

message(STATUS "no D-Bus activation of veyon-server")
