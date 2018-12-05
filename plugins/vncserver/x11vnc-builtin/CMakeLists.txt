INCLUDE(BuildPlugin)

IF(VEYON_X11VNC_EXTERNAL)

ADD_DEFINITIONS(-DVEYON_X11VNC_EXTERNAL)

ELSE()

# check for pthreads and TLS support
SET(CMAKE_THREAD_PREFER_PTHREAD TRUE)
FIND_PACKAGE(Threads)
IF(CMAKE_USE_PTHREADS_INIT)
    SET(LIBVNCSERVER_HAVE_LIBPTHREAD TRUE)
ENDIF(CMAKE_USE_PTHREADS_INIT)

# check for libvncserver requirements

# functions
SET(LIBVNCSERVER_FUNCS fork ftime gethostbyname gethostname gettimeofday inet_ntoa memmove memset mkfifo mmap select socket strchr strdup strerror strstr vfork vprintf)
FOREACH(_func ${LIBVNCSERVER_FUNCS})
    STRING(TOUPPER "${_func}" fuc)
	CHECK_FUNCTION_EXISTS(${_func} LIBVNCSERVER_HAVE_${fuc})
ENDFOREACH()

# headers
SET(LIBVNCSERVER_HEADERS fcntl sys/endian sys/wait sys/uio vfork)
FOREACH(_header ${LIBVNCSERVER_HEADERS})
    STRING(TOUPPER "${_header}" _huc)
	STRING(REPLACE "/" "_" _header_escaped "${_huc}")
	CHECK_INCLUDE_FILES(${_header}.h LIBVNCSERVER_HAVE_${_header_escaped}_H)
ENDFOREACH()

SET(LIBVNCSERVER_ALLOW24BPP TRUE)
SET(FULL_PACKAGE_NAME "Veyon")
SET(PACKAGE_VERSION "${VERSION_STRING}")
SET(VERSION_PATCHLEVEL "${VERSION_PATCH}")

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/3rdparty/libvncserver/rfb/rfbconfig.h.cmakein ${CMAKE_BINARY_DIR}/${VEYON_CORE_INCLUDE_DIR}/rfb/rfbconfig.h @ONLY)

# check for x11vnc requirements
SET(FUNCS dup2 floor ftime geteuid gethostbyname gethostname getpwnam getpwuid getspnam gettimeofday getuid grantpt inet_ntoa initgroups memcmp memcpy memmove memset mkfifo mmap fork pow putenv select seteuid setegid setgid setpgrp setsid setuid setutxent shmat socket strchr strcspn strdup strerror strftime strpbrk strrchr strstr uname vfork vprintf waitpid)
FOREACH(_func ${FUNCS})
    STRING(TOUPPER "${_func}" fuc)
	CHECK_FUNCTION_EXISTS(${_func} HAVE_${fuc})
ENDFOREACH(_func ${FUNCS})

CHECK_C_SOURCE_COMPILES("static __thread int p = 0; int main() {}" HAVE_TLS)

CHECK_INCLUDE_FILES(arpa/inet.h HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILES(ctype.h HAVE_CTYPE_H)
CHECK_INCLUDE_FILES(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILES(endian.h HAVE_ENDIAN_H)
CHECK_INCLUDE_FILES(errno.h HAVE_ERRNO_H)
CHECK_INCLUDE_FILES(fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H)
CHECK_INCLUDE_FILES(linux/fb.h HAVE_LINUX_FB_H)
CHECK_INCLUDE_FILES(linux/input.h HAVE_LINUX_INPUT_H)
CHECK_INCLUDE_FILES(linux/uinput.h HAVE_LINUX_UINPUT_H)
CHECK_INCLUDE_FILES(linux/videodev2.h HAVE_LINUX_VIDEODEV2_H)
CHECK_INCLUDE_FILES(linux/videodev.h HAVE_LINUX_VIDEODEV_H)
CHECK_INCLUDE_FILES(memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES(netdb.h HAVE_NETDB_H)
CHECK_INCLUDE_FILES(netinet/in.h HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILES(process.h HAVE_PROCESS_H)
CHECK_INCLUDE_FILES(pthread.h HAVE_PTHREAD_H)
CHECK_INCLUDE_FILES(pwd.h HAVE_PWD_H)
CHECK_INCLUDE_FILES(signal.h HAVE_SIGNAL_H)
CHECK_INCLUDE_FILES(stdarg.h HAVE_STDARG_H)
CHECK_INCLUDE_FILES(stdbool.h HAVE_STDBOOL_H)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H)
CHECK_INCLUDE_FILES(stdlib.h HAVE_STDLIB_H)
CHECK_INCLUDE_FILES(string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(sys/endian.h HAVE_SYS_ENDIAN_H)
CHECK_INCLUDE_FILES(sys/ioctl.h HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILES(sys/ipc.h HAVE_SYS_IPC_H)
CHECK_INCLUDE_FILES(syslog.h HAVE_SYSLOG_H)
CHECK_INCLUDE_FILES(sys/shm.h HAVE_SYS_SHM_H)
CHECK_INCLUDE_FILES(sys/socket.h HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILES(sys/stat.h HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILES(sys/stropts.h HAVE_SYS_STROPTS_H)
CHECK_INCLUDE_FILES(sys/timeb.h HAVE_SYS_TIMEB_H)
CHECK_INCLUDE_FILES(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILES(sys/wait.h HAVE_SYS_WAIT_H)
CHECK_INCLUDE_FILES(termios.h HAVE_TERMIOS_H)
CHECK_INCLUDE_FILES(time.h HAVE_TIME_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES(utmpx.h HAVE_UTMPX_H)
CHECK_INCLUDE_FILES(vfork.h HAVE_VFORK_H)

FIND_PACKAGE(X11 REQUIRED)

IF(NOT X11_XTest_FOUND)
	MESSAGE(FATAL_ERROR "XTest library or headers not found - please install libxtst-dev or libXtst-devel")
ENDIF()

IF(NOT X11_Xrandr_FOUND)
	MESSAGE(FATAL_ERROR "Xrandr library or headers not found - please install libxrandr-dev or libXrandr-devel")
ENDIF()

IF(NOT X11_Xinerama_FOUND)
	MESSAGE(FATAL_ERROR "Xinerama library or headers not found - please install libxinerama-dev or libXinerama-devel")
ENDIF()

IF(NOT X11_Xdamage_FOUND)
	MESSAGE(FATAL_ERROR "Xdamage library or headers not found - please install libxdamage-dev or libXdamage-devel")
ENDIF()

IF(NOT X11_Xfixes_FOUND)
	MESSAGE(FATAL_ERROR "Xfixes library or headers not found - please install libxfixes-dev or libXfixes-devel")
ENDIF()

SET(HAVE_X11 TRUE)
SET(HAVE_XTEST TRUE)
SET(HAVE_LIBSSL TRUE)
SET(HAVE_LIBXINERAMA TRUE)
SET(HAVE_LIBXRANDR TRUE)
SET(HAVE_LIBXDAMAGE TRUE)
SET(HAVE_LIBXFIXES TRUE)

IF(X11_XShm_FOUND)
    SET(HAVE_XSHM TRUE)
ELSE()
	MESSAGE("WARNING: XShm library or headers not found - building VNC server without XShm support")
ENDIF()

IF(X11_Xinput_FOUND)
    SET(HAVE_XI2 TRUE)
ELSE()
	MESSAGE("WARNING: Xinput library or headers not found - building VNC server without Xinput support")
ENDIF()

IF(X11_Xcomposite_FOUND)
    SET(HAVE_LIBXCOMPOSITE TRUE)
ELSE()
	MESSAGE("WARNING: Xcomposite library or headers not found - building VNC server without Xcomposite support")
ENDIF()

IF(X11_Xcursor_FOUND)
    SET(HAVE_LIBXCURSOR TRUE)
ELSE()
	MESSAGE("WARNING: Xcursor library or headers not found - building VNC server without Xcursor support")
ENDIF()

SET(CMAKE_REQUIRED_LIBRARIES ${X11_LIBRARIES} ${X11_XTest_LIB})

CHECK_FUNCTION_EXISTS(XReadScreen HAVE_SOLARIS_XREADSCREEN)
CHECK_FUNCTION_EXISTS(FBPMForceLevel HAVE_FBPM)
CHECK_FUNCTION_EXISTS(DPMSForceLevel HAVE_DPMS)
CHECK_FUNCTION_EXISTS(XTestGrabControl HAVE_XTESTGRABCONTROL)
CHECK_FUNCTION_EXISTS(XRecordEnableContextAsync HAVE_RECORD)
CHECK_INCLUDE_FILES(X11/extensions/readdisplay.h HAVE_IRIX_XREADDISPLAY)
CHECK_INCLUDE_FILES(X11/XKBlib.h HAVE_XKBLIB_H)
IF(HAVE_XKBLIB_H)
    CHECK_FUNCTION_EXISTS(XkbSelectEvents HAVE_XKEYBOARD)
ENDIF(HAVE_XKBLIB_H)

UNSET(CMAKE_REQUIRED_LIBRARIES)

SET(X11VNC_CONFIG ${CMAKE_BINARY_DIR}/config.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

ADD_DEFINITIONS(-DVNCSHARED -DFOREVER -DNOREPEAT=0 -DNOPW=1 -DREMOTE_CONTROL=0 -DEXTERNAL_COMMANDS=0 -DFILEXFER=0 -DNOGUI -DSMALL_FOOTPRINT)
INCLUDE_DIRECTORIES(${libvncserver_DIR}/libvncserver ${libvncserver_DIR}/common ${3rdparty_DIR} ${x11vnc_DIR}/src)

SET(libvncserver_SOURCES
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

SET(x11vnc_SOURCES x11vnc-veyon.c
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

ENDIF()

BUILD_PLUGIN(builtin-x11vnc-server
	BuiltinX11VncServer.cpp
	X11VncConfiguration.cpp
	X11VncConfigurationWidget.cpp
	${libvncserver_SOURCES}
	${x11vnc_SOURCES}
	MOCFILES
	BuiltinX11VncServer.h
	X11VncConfigurationWidget.h
	X11VncConfiguration.h
	FORMS X11VncConfigurationWidget.ui)

IF(NOT VEYON_X11VNC_EXTERNAL)
TARGET_LINK_LIBRARIES(builtin-x11vnc-server
	Threads::Threads
	${X11_LIBRARIES}
	${X11_XTest_LIB}
	${X11_Xfixes_LIB}
	${X11_Xinerama_LIB}
	${X11_Xdamage_LIB}
	${X11_Xrandr_LIB}
)

IF(X11_XShm_FOUND)
TARGET_LINK_LIBRARIES(builtin-x11vnc-server ${X11_XShm_LIB})
ENDIF()

IF(X11_Xcomposite_FOUND)
TARGET_LINK_LIBRARIES(builtin-x11vnc-server ${X11_Xcomposite_LIB})
ENDIF()

IF(X11_Xcursor_FOUND)
TARGET_LINK_LIBRARIES(builtin-x11vnc-server ${X11_Xcursor_LIB})
ENDIF()

IF(X11_Xinput_FOUND)
TARGET_LINK_LIBRARIES(builtin-x11vnc-server ${X11_Xinput_LIB})
ENDIF()

ENDIF()


SET_SOURCE_FILES_PROPERTIES(${x11vnc_SOURCES} ${libvncserver_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-result -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-misleading-indentation -Wno-deprecated-declarations -Wno-address -Wno-format -Wno-discarded-qualifiers")
