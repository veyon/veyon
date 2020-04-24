file(GLOB veyoncore_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.ui
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.h
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/builddata.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc)
set(core_RESOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/resources/core.qrc
	${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc
)

set(libvncclient_SOURCES
	${libvncserver_DIR}/libvncclient/cursor.c
	${libvncserver_DIR}/libvncclient/listen.c
	${libvncserver_DIR}/libvncclient/rfbproto.c
	${libvncserver_DIR}/libvncclient/sockets.c
	${libvncserver_DIR}/libvncclient/tls_openssl.c
	${libvncserver_DIR}/libvncclient/vncviewer.c
	${libvncserver_DIR}/common/crypto_openssl.c
	${libvncserver_DIR}/common/d3des.c
	${libvncserver_DIR}/common/turbojpeg.c)

set_source_files_properties(${libvncclient_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unused-variable -Wno-incompatible-pointer-types -fvisibility=default")

add_library(veyon-core SHARED ${veyoncore_SOURCES} ${core_RESOURCES} ${libvncclient_SOURCES})

set_default_target_properties(veyon-core)

target_include_directories(veyon-core PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_BINARY_DIR}/src
	${libvncserver_DIR}/common/
	${libvncserver_DIR}
	${QCA_INCLUDE_DIR}
)

target_include_directories(veyon-core PRIVATE SYSTEM ${ZLIB_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${LZO_INCLUDE_DIR})

target_compile_options(veyon-core PRIVATE ${VEYON_COMPILE_OPTIONS})

# find Qt's translation directory
if(NOT VEYON_BUILD_WIN32)
	include(FindQtTranslations)
	find_qt_translations()
	target_compile_definitions(veyon-core PRIVATE QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}")
endif()


target_link_libraries(veyon-core
	Qt5::Concurrent
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	Qt5::QuickControls2
	Threads::Threads
	${VEYON_DEBUG_LIBRARIES}
	${ZLIB_LIBRARIES}
	${JPEG_LIBRARIES}
	${PNG_LIBRARIES}
	${LZO_LIBRARIES}
	${QCA_LIBRARY}
	${OPENSSL_LIBRARIES}
	)

if(VEYON_BUILD_WIN32)
	# add Windows Socket library required by libvncclient
	target_link_libraries(veyon-core -lws2_32)
	set_target_properties(veyon-core PROPERTIES PREFIX "")
	install(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
else()
	install(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
endif()

if(VEYON_BUILD_ANDROID)
	target_link_libraries(veyon-core Qt5::AndroidExtras)
endif()

cotire_veyon(veyon-core)
