add_subdirectory(vnchooks)

include(BuildVeyonPlugin)

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(LZO REQUIRED)

set(ultravnc_CXX_SOURCES
	${ultravnc_DIR}/winvnc/winvnc/HideDesktop.cpp
	${ultravnc_DIR}/winvnc/winvnc/rfbRegion_win32.cpp
	${ultravnc_DIR}/winvnc/winvnc/vistahook.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncdesktopthread.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncdesktopsink.cpp
	${ultravnc_DIR}/winvnc/winvnc/IPC.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncencoderre.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncdesktop.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncserver.cpp
	${ultravnc_DIR}/winvnc/winvnc/rfbUpdateTracker.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncencodehext.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncproperties.cpp
	${ultravnc_DIR}/winvnc/winvnc/security.cpp
	${ultravnc_DIR}/winvnc/winvnc/buildtime.cpp
	${ultravnc_DIR}/winvnc/winvnc/Timer.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncencoderCursor.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncencoder.cpp
	${ultravnc_DIR}/winvnc/winvnc/vnclog.cpp
	${ultravnc_DIR}/winvnc/winvnc/translate.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncencodecorre.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncencodezrle.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncEncodeTight.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncservice.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncMultiMonitor.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncbuffer.cpp
	${ultravnc_DIR}/winvnc/winvnc/videodrivercheck.cpp
	${ultravnc_DIR}/winvnc/winvnc/videodriver.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncDesktopSW.cpp
	${ultravnc_DIR}/winvnc/winvnc/vnckeymap.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncOSVersion.cpp
	${ultravnc_DIR}/winvnc/winvnc/winvnc.cpp
	${ultravnc_DIR}/winvnc/winvnc/stdhdrs.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncEncodeUltra.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncEncodeUltra2.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncsockconnect.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncinsthandler.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncEncodeZlib.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncEncodeZlibHex.cpp
	${ultravnc_DIR}/winvnc/winvnc/vncpropertiesPoll.cpp
	${ultravnc_DIR}/winvnc/winvnc/helpers.cpp
	${ultravnc_DIR}/winvnc/winvnc/CpuUsage.cpp
	${ultravnc_DIR}/winvnc/winvnc/uvncUiAccess.cpp
	${ultravnc_DIR}/winvnc/winvnc/ScreenCapture.cpp
	${ultravnc_DIR}/winvnc/winvnc/DeskdupEngine.cpp
	${ultravnc_DIR}/winvnc/winvnc/vsocket.cpp
	${ultravnc_DIR}/winvnc/omnithread/nt.cpp
	${ultravnc_DIR}/common/Clipboard.cpp
	${ultravnc_DIR}/common/win32_helpers.cpp
	${ultravnc_DIR}/common/UltraVncZ.cpp
	${ultravnc_DIR}/rfb/dh.cpp
	${ultravnc_DIR}/rdr/ZlibOutStream.cxx
	${ultravnc_DIR}/rdr/ZlibInStream.cxx
	ultravnc.cpp
	vncntlm.cpp
	)

set(ultravnc_C_SOURCES
	${ultravnc_DIR}/winvnc/winvnc/d3des.c
	${ultravnc_DIR}/winvnc/winvnc/vncauth.c
	)

set(WITH_PCH OFF)

build_veyon_plugin(builtin-ultravnc-server
	BuiltinUltraVncServer.cpp
	LogoffEventFilter.cpp
	UltraVncConfigurationWidget.cpp
	UltraVncConfigurationWidget.ui
	${ultravnc_C_SOURCES}
	${ultravnc_CXX_SOURCES}
	BuiltinUltraVncServer.h
	LogoffEventFilter.h
	UltraVncConfigurationWidget.h
	UltraVncConfiguration.h
)

target_link_libraries(builtin-ultravnc-server -luserenv -lole32 -lversion -lgdi32 -limm32 -lwinmm ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${LZO_LIBRARIES})
target_include_directories(builtin-ultravnc-server PRIVATE
	${ultravnc_DIR}
	${ultravnc_DIR}/winvnc
	${ultravnc_DIR}/winvnc/omnithread
	${ultravnc_DIR}/winvnc/winvnc
	)

target_compile_definitions(builtin-ultravnc-server PRIVATE ULTRAVNC_VEYON_SUPPORT _INTERNALLIB)

if(VEYON_BUILD_WIN64)
	target_compile_definitions(builtin-ultravnc-server PRIVATE _X64)
endif()

set(ULTRAVNC_COMPILER_FLAGS "-Wno-comments -Wno-attributes -Wno-write-strings -Wno-parentheses -Wno-misleading-indentation -Wno-unused-result -Wno-unused-label -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-but-set-variable -Wno-deprecated-declarations -Wno-format-zero-length -Wno-sign-compare -fexceptions")

set_source_files_properties(${ultravnc_C_SOURCES} PROPERTIES COMPILE_FLAGS "${ULTRAVNC_COMPILER_FLAGS}")
set_source_files_properties(${ultravnc_CXX_SOURCES} PROPERTIES COMPILE_FLAGS "${ULTRAVNC_COMPILER_FLAGS} -Wno-terminate -Wno-conversion-null")
