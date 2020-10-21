option(WITH_WAYLAND_SUPPORT "Enable Wayland support via builtin-wayvnc-server plugin" ON)

if(WITH_WAYLAND_SUPPORT)

option(WITH_WAYVNC_DMABUF "Enable screencopy-dmabuf support for wayvnc" OFF)
option(WITH_WAYVNC_TLS "Enable TLS support for wayvnc" OFF)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

include(BuildVeyonPlugin)
include(CheckFunctionExists)
include(CheckIncludeFile)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(Pixman REQUIRED)
find_package(PNG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Wayland REQUIRED)
find_package(WaylandScanner REQUIRED)
find_package(XKBCommon REQUIRED)

if(WITH_WAYVNC_TLS)
	find_package(GnuTLS REQUIRED)
	set(ENABLE_TLS TRUE)
endif()

if(WITH_WAYVNC_DMABUF)
	find_package(GBM REQUIRED)
	find_package(DRM REQUIRED)
	set(ENABLE_SCREENCOPY_DMABUF TRUE)
endif()

check_include_file("sys/sdt.h" HAVE_USDT)
check_function_exists(memfd_create HAVE_MEMFD)
check_function_exists(memfd_create HAVE_MEMFD_CREATE)

configure_file(config.h.in config.h)

file(GLOB protocols ${wayvnc_DIR}/protocols/*.xml)
foreach(protocol ${protocols})
	get_filename_component(protocol_basename ${protocol} NAME_WE)
	ecm_add_wayland_client_protocol(CLIENT_PROTOCOL_SRCS PROTOCOL ${protocol} BASENAME ${protocol_basename})
endforeach()

set(wayvnc_SOURCES
	${aml_DIR}/src/aml.c
	${aml_DIR}/src/epoll.c
	${aml_DIR}/src/posix.c
	${aml_DIR}/src/thread-pool.c
	${neatvnc_DIR}/src/damage.c
	${neatvnc_DIR}/src/display.c
	${neatvnc_DIR}/src/enc-util.c
	${neatvnc_DIR}/src/fb.c
	${neatvnc_DIR}/src/pixels.c
	${neatvnc_DIR}/src/pngfb.c
	${neatvnc_DIR}/src/raw-encoding.c
	${neatvnc_DIR}/src/rcbuf.c
	${neatvnc_DIR}/src/server.c
	${neatvnc_DIR}/src/stream.c
	${neatvnc_DIR}/src/tight.c
	${neatvnc_DIR}/src/vec.c
	${neatvnc_DIR}/src/zrle.c
	${wayvnc_DIR}/src/buffer.c
	${wayvnc_DIR}/src/damage-refinery.c
	${wayvnc_DIR}/src/data-control.c
	${wayvnc_DIR}/src/intset.c
	${wayvnc_DIR}/src/keyboard.c
	${wayvnc_DIR}/src/main.c
	${wayvnc_DIR}/src/murmurhash.c
	${wayvnc_DIR}/src/output.c
	${wayvnc_DIR}/src/pixels.c
	${wayvnc_DIR}/src/pixman-renderer.c
	${wayvnc_DIR}/src/pointer.c
	${wayvnc_DIR}/src/screencopy.c
	${wayvnc_DIR}/src/seat.c
	${wayvnc_DIR}/src/shm.c
	${wayvnc_DIR}/src/smooth.c
	${wayvnc_DIR}/src/strlcpy.c
	${wayvnc_DIR}/src/transform-util.c
	${CLIENT_PROTOCOL_SRCS}
)

set_source_files_properties(${wayvnc_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-implicit-function-declaration -Dmain=wayvnc_main")

build_veyon_plugin(builtin-wayvnc-server
	BuiltinWayVncServer.cpp
	BuiltinWayVncServer.h
	WayVncConfiguration.h
	WayVncConfigurationWidget.cpp
	WayVncConfigurationWidget.h
	WayVncConfigurationWidget.ui
	${wayvnc_SOURCES}
)

target_link_libraries(builtin-wayvnc-server
	Threads::Threads
	PNG::PNG
	Wayland::Client
	${ZLIB_LIBRARIES}
	${PIXMAN_LIBRARIES}
	${XKBCOMMON_LIBRARIES}
)

if(WITH_WAYVNC_TLS)
	target_link_libraries(builtin-wayvnc-server PRIVATE GnuTLS::GnuTLS)
endif()

if(WITH_WAYVNC_DMABUF)
	target_include_directories(builtin-wayvnc-server PRIVATE ${DRM_INCLUDE_DIRS})
	target_link_libraries(builtin-wayvnc-server PRIVATE gbm::gbm ${DRM_LIBRARIES})
endif()

target_include_directories(builtin-wayvnc-server PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${ZLIB_INCLUDE_DIR}
	${PIXMAN_INCLUDE_DIR}
	${XKBCOMMON_INCLUDE_DIRS}
	${aml_DIR}/include
	${neatvnc_DIR}/include
	${wayvnc_DIR}/include
)

target_compile_definitions(builtin-wayvnc-server PRIVATE -D_GNU_SOURCE)

endif()
