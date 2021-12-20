include(BuildVeyonPlugin)

if(NOT VEYON_X11VNC_EXTERNAL)

	get_property(HAVE_LIBVNCCLIENT GLOBAL PROPERTY HAVE_LIBVNCCLIENT)
	if(HAVE_LIBVNCCLIENT)
		find_package(LibVNCServer 0.9.8)
	else()
		message(WARNING "No suitable LibVNCClient library found therefore performing internal build of LibVNCServer as well to avoid binary compatibility issues")
	endif()

	if(NOT LibVNCServer_FOUND)
		include(LibVNCServerIntegration)
	endif()

	set(FULL_PACKAGE_NAME "Veyon")
	set(PACKAGE_VERSION "${VERSION_STRING}")
	set(VERSION_PATCHLEVEL "${VERSION_PATCH}")

	# check for x11vnc requirements
	set(FUNCS getpwnam getspnam getuid grantpt initgroups seteuid setegid setgid setsid setuid shmat waitpid)
	foreach(_func ${FUNCS})
		string(TOUPPER "${_func}" fuc)
		check_function_exists(${_func} HAVE_${fuc})
	endforeach()

	check_c_source_compiles("static __thread int p = 0; int main() {}" HAVE_TLS)

	# x11vnc header macros
	check_include_files(linux/fb.h HAVE_LINUX_FB_H)
	check_include_files(linux/input.h HAVE_LINUX_INPUT_H)
	check_include_files(linux/uinput.h HAVE_LINUX_UINPUT_H)
	check_include_files(linux/videodev.h HAVE_LINUX_VIDEODEV_H)
	check_include_files(linux/videodev2.h HAVE_LINUX_VIDEODEV2_H)
	check_include_files(netdb.h HAVE_NETDB_H)
	check_include_files(netinet/in.h HAVE_NETINET_IN_H)
	check_include_files(pwd.h HAVE_PWD_H)
	check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
	check_include_files(sys/stropts.h HAVE_SYS_STROPTS_H)
	check_include_files(sys/wait.h HAVE_SYS_WAIT_H)
	check_include_files(termios.h HAVE_TERMIOS_H)
	check_include_files(utmpx.h HAVE_UTMPX_H)

	find_package(X11 REQUIRED)

	if(NOT X11_XTest_FOUND)
		message(FATAL_ERROR "XTest library or headers not found - please install libxtst-dev or libXtst-devel")
	endif()

	if(NOT X11_Xrandr_FOUND)
		message(FATAL_ERROR "Xrandr library or headers not found - please install libxrandr-dev or libXrandr-devel")
	endif()

	if(NOT X11_Xinerama_FOUND)
		message(FATAL_ERROR "Xinerama library or headers not found - please install libxinerama-dev or libXinerama-devel")
	endif()

	if(NOT X11_Xdamage_FOUND)
		message(FATAL_ERROR "Xdamage library or headers not found - please install libxdamage-dev or libXdamage-devel")
	endif()

	if(NOT X11_Xfixes_FOUND)
		message(FATAL_ERROR "Xfixes library or headers not found - please install libxfixes-dev or libXfixes-devel")
	endif()

	set(HAVE_X11 TRUE)
	set(HAVE_XTEST TRUE)
	set(HAVE_LIBSSL TRUE)
	set(HAVE_LIBXINERAMA TRUE)
	set(HAVE_LIBXRANDR TRUE)
	set(HAVE_LIBXDAMAGE TRUE)
	set(HAVE_LIBXFIXES TRUE)

	if(X11_XShm_FOUND)
		set(HAVE_XSHM TRUE)
	else()
		message("WARNING: XShm library or headers not found - building VNC server without XShm support")
	endif()

	if(X11_Xinput_FOUND)
		set(HAVE_XI2 TRUE)
	else()
		message("WARNING: Xinput library or headers not found - building VNC server without Xinput support")
	endif()

	if(X11_Xcomposite_FOUND)
		set(HAVE_LIBXCOMPOSITE TRUE)
	else()
		message("WARNING: Xcomposite library or headers not found - building VNC server without Xcomposite support")
	endif()

	if(X11_Xcursor_FOUND)
		set(HAVE_LIBXCURSOR TRUE)
	else()
		message("WARNING: Xcursor library or headers not found - building VNC server without Xcursor support")
	endif()

	set(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES} ${X11_XTest_LIB})

	check_function_exists(XReadScreen HAVE_SOLARIS_XREADSCREEN)
	check_function_exists(FBPMForceLevel HAVE_FBPM)
	check_function_exists(DPMSForceLevel HAVE_DPMS)
	check_function_exists(XTestGrabControl HAVE_XTESTGRABCONTROL)
	check_function_exists(XRecordEnableContextAsync HAVE_RECORD)
	check_include_files(X11/extensions/readdisplay.h HAVE_IRIX_XREADDISPLAY)
	check_include_files(X11/XKBlib.h HAVE_XKBLIB_H)
	if(HAVE_XKBLIB_H)
		check_function_exists(XkbSelectEvents HAVE_XKEYBOARD)
	endif()

	set(CMAKE_REQUIRED_LIBRARIES ${OPENSSL_LIBRARIES})
	check_function_exists(X509_print_ex_fp HAVE_X509_PRINT_EX_FP)

	unset(CMAKE_REQUIRED_LIBRARIES)

	set(X11VNC_CONFIG ${CMAKE_BINARY_DIR}/config.h)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
	if(NOT LibVNCServer_FOUND)
		set(libvncserver_SOURCES
			${libvncserver_DIR}/libvncserver/auth.c
			${libvncserver_DIR}/libvncserver/cargs.c
			${libvncserver_DIR}/libvncserver/corre.c
			${libvncserver_DIR}/libvncserver/cursor.c
			${libvncserver_DIR}/libvncserver/cutpaste.c
			${libvncserver_DIR}/libvncserver/draw.c
			${libvncserver_DIR}/libvncserver/font.c
			${libvncserver_DIR}/libvncserver/hextile.c
			${libvncserver_DIR}/libvncserver/httpd.c
			${libvncserver_DIR}/libvncserver/main.c
			${libvncserver_DIR}/libvncserver/rfbregion.c
			${libvncserver_DIR}/libvncserver/rfbserver.c
			${libvncserver_DIR}/libvncserver/rre.c
			${libvncserver_DIR}/libvncserver/scale.c
			${libvncserver_DIR}/libvncserver/selbox.c
			${libvncserver_DIR}/libvncserver/sockets.c
			${libvncserver_DIR}/libvncserver/stats.c
			${libvncserver_DIR}/libvncserver/translate.c
			${libvncserver_DIR}/libvncserver/ultra.c
			${libvncserver_DIR}/libvncserver/zlib.c
			${libvncserver_DIR}/libvncserver/zrle.c
			${libvncserver_DIR}/libvncserver/zrleoutstream.c
			${libvncserver_DIR}/libvncserver/zrlepalettehelper.c
			${libvncserver_DIR}/libvncserver/tight.c
			${libvncserver_DIR}/common/d3des.c
			${libvncserver_DIR}/common/turbojpeg.c
			${libvncserver_DIR}/common/vncauth.c)
	endif()

	set(x11vnc_SOURCES x11vnc-veyon.c
		${x11vnc_DIR}/src/appshare.c
		${x11vnc_DIR}/src/avahi.c
		${x11vnc_DIR}/src/rates.c
		${x11vnc_DIR}/src/cleanup.c
		${x11vnc_DIR}/src/remote.c
		${x11vnc_DIR}/src/pointer.c
		${x11vnc_DIR}/src/userinput.c
		${x11vnc_DIR}/src/unixpw.c
		${x11vnc_DIR}/src/gui.c
		${x11vnc_DIR}/src/xkb_bell.c
		${x11vnc_DIR}/src/xinerama.c
		${x11vnc_DIR}/src/solid.c
		${x11vnc_DIR}/src/selection.c
		${x11vnc_DIR}/src/xrandr.c
		${x11vnc_DIR}/src/win_utils.c
		${x11vnc_DIR}/src/cursor.c
		${x11vnc_DIR}/src/screen.c
		${x11vnc_DIR}/src/xevents.c
		${x11vnc_DIR}/src/help.c
		${x11vnc_DIR}/src/inet.c
		${x11vnc_DIR}/src/sslcmds.c
		${x11vnc_DIR}/src/xwrappers.c
		${x11vnc_DIR}/src/scan.c
		${x11vnc_DIR}/src/options.c
		${x11vnc_DIR}/src/user.c
		${x11vnc_DIR}/src/util.c
		${x11vnc_DIR}/src/x11vnc_defs.c
		${x11vnc_DIR}/src/xrecord.c
		${x11vnc_DIR}/src/8to24.c
		${x11vnc_DIR}/src/xdamage.c
		${x11vnc_DIR}/src/keyboard.c
		${x11vnc_DIR}/src/connections.c
		${x11vnc_DIR}/src/sslhelper.c
		${x11vnc_DIR}/src/linuxfb.c
		${x11vnc_DIR}/src/v4l.c
		${x11vnc_DIR}/src/macosx.c
		${x11vnc_DIR}/src/macosxCG.c
		${x11vnc_DIR}/src/macosxCGP.c
		${x11vnc_DIR}/src/macosxCGS.c
		${x11vnc_DIR}/src/xi2_devices.c
		${x11vnc_DIR}/src/uinput.c
		)

	set_source_files_properties(${x11vnc_SOURCES} ${libvncserver_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-result -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-deprecated-declarations -Wno-address -Wno-discarded-qualifiers -Wno-strict-aliasing -Wno-restrict -Wno-multistatement-macros -Wno-stringop-truncation -Wno-stringop-overflow -Wno-format-overflow")

endif()

set(WITH_PCH OFF)

build_veyon_plugin(builtin-x11vnc-server
	BuiltinX11VncServer.cpp
	X11VncConfigurationWidget.cpp
	X11VncConfigurationWidget.ui
	${libvncserver_SOURCES}
	${x11vnc_SOURCES}
	BuiltinX11VncServer.h
	X11VncConfigurationWidget.h
	X11VncConfiguration.h
	)

if(VEYON_X11VNC_EXTERNAL)

	target_compile_definitions(builtin-x11vnc-server PRIVATE VEYON_X11VNC_EXTERNAL)

else()

	target_compile_definitions(builtin-x11vnc-server PRIVATE VNCSHARED FOREVER NOREPEAT=0 NOPW=1 REMOTE_CONTROL=0 EXTERNAL_COMMANDS=0 FILEXFER=0 NOGUI SMALL_FOOTPRINT)

	target_include_directories(builtin-x11vnc-server PRIVATE ${x11vnc_DIR}/src)

	target_link_libraries(builtin-x11vnc-server PRIVATE
		Threads::Threads
		PNG::PNG
		${ZLIB_LIBRARIES}
		${JPEG_LIBRARIES}
		${LZO_LIBRARIES}
		${X11_LIBRARIES}
		${X11_XTest_LIB}
		${X11_Xfixes_LIB}
		${X11_Xinerama_LIB}
		${X11_Xdamage_LIB}
		${X11_Xrandr_LIB}
		)

	if(LibVNCServer_FOUND)
		target_link_libraries(builtin-x11vnc-server PRIVATE LibVNC::LibVNCServer)
	else()
		target_include_directories(builtin-x11vnc-server PRIVATE ${libvncserver_DIR}/libvncserver ${libvncserver_DIR}/common)
	endif()

	if(X11_XShm_FOUND)
		target_link_libraries(builtin-x11vnc-server PRIVATE ${X11_XShm_LIB})
	endif()

	if(X11_Xcomposite_FOUND)
		target_link_libraries(builtin-x11vnc-server PRIVATE ${X11_Xcomposite_LIB})
	endif()

	if(X11_Xcursor_FOUND)
		target_link_libraries(builtin-x11vnc-server PRIVATE ${X11_Xcursor_LIB})
	endif()

	if(X11_Xinput_FOUND)
		target_link_libraries(builtin-x11vnc-server PRIVATE ${X11_Xinput_LIB})
	endif()

endif()
