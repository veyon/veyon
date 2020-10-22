if(VEYON_BUILD_WIN32)
add_subdirectory(ultravnc-builtin)
endif()

if(VEYON_BUILD_LINUX)
add_subdirectory(pipewire)
add_subdirectory(wayvnc-builtin)
add_subdirectory(x11vnc-builtin)
endif()

add_subdirectory(external)
