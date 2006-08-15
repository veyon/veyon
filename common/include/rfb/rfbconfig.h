#ifndef _COMMON_INCLUDE_RFB_RFBCONFIG_H
#define _COMMON_INCLUDE_RFB_RFBCONFIG_H 1
 
/* common/include/rfb/rfbconfig.h. Generated automatically at end of configure. */
/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Enable 24 bit per pixel in native framebuffer */
#ifndef LIBVNCSERVER_ALLOW24BPP 
#define LIBVNCSERVER_ALLOW24BPP  1 
#endif

/* Enable BackChannel communication */
#ifndef LIBVNCSERVER_BACKCHANNEL 
#define LIBVNCSERVER_BACKCHANNEL  1 
#endif

/* Build iTALC for Linux */
#ifndef LIBVNCSERVER_BUILD_LINUX 
#define LIBVNCSERVER_BUILD_LINUX  1 
#endif

/* Build iTALC for Win32 */
/* #undef LIBVNCSERVER_BUILD_WIN32 */

/* work around when write() returns ENOENT but does not mean it */
/* #undef LIBVNCSERVER_ENOENT_WORKAROUND */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#ifndef LIBVNCSERVER_HAVE_ARPA_INET_H 
#define LIBVNCSERVER_HAVE_ARPA_INET_H  1 
#endif

/* Define to 1 if you have the `crypt' function. */
/* #undef LIBVNCSERVER_HAVE_CRYPT */

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef LIBVNCSERVER_HAVE_DOPRNT */

/* Define to 1 if you have the openssl/dsa.h header file. */
#ifndef LIBVNCSERVER_HAVE_DSA_H 
#define LIBVNCSERVER_HAVE_DSA_H  1 
#endif

/* Define to 1 if you have the `dup2' function. */
#ifndef LIBVNCSERVER_HAVE_DUP2 
#define LIBVNCSERVER_HAVE_DUP2  1 
#endif

/* Define to 1 if you have the <errno.h> header file. */
#ifndef LIBVNCSERVER_HAVE_ERRNO_H 
#define LIBVNCSERVER_HAVE_ERRNO_H  1 
#endif

/* FBPM extension build environment present */
/* #undef LIBVNCSERVER_HAVE_FBPM */

/* Define to 1 if you have the <fcntl.h> header file. */
#ifndef LIBVNCSERVER_HAVE_FCNTL_H 
#define LIBVNCSERVER_HAVE_FCNTL_H  1 
#endif

/* Define to 1 if you have the `floor' function. */
/* #undef LIBVNCSERVER_HAVE_FLOOR */

/* Define to 1 if you have the `fork' function. */
#ifndef LIBVNCSERVER_HAVE_FORK 
#define LIBVNCSERVER_HAVE_FORK  1 
#endif

/* Define to 1 if you have the `ftime' function. */
#ifndef LIBVNCSERVER_HAVE_FTIME 
#define LIBVNCSERVER_HAVE_FTIME  1 
#endif

/* Define to 1 if you have the `geteuid' function. */
#ifndef LIBVNCSERVER_HAVE_GETEUID 
#define LIBVNCSERVER_HAVE_GETEUID  1 
#endif

/* Define to 1 if you have the `gethostbyname' function. */
#ifndef LIBVNCSERVER_HAVE_GETHOSTBYNAME 
#define LIBVNCSERVER_HAVE_GETHOSTBYNAME  1 
#endif

/* Define to 1 if you have the `gethostname' function. */
#ifndef LIBVNCSERVER_HAVE_GETHOSTNAME 
#define LIBVNCSERVER_HAVE_GETHOSTNAME  1 
#endif

/* Define to 1 if you have the `getpwnam' function. */
#ifndef LIBVNCSERVER_HAVE_GETPWNAM 
#define LIBVNCSERVER_HAVE_GETPWNAM  1 
#endif

/* Define to 1 if you have the `getpwuid' function. */
#ifndef LIBVNCSERVER_HAVE_GETPWUID 
#define LIBVNCSERVER_HAVE_GETPWUID  1 
#endif

/* Define to 1 if you have the `getspnam' function. */
#ifndef LIBVNCSERVER_HAVE_GETSPNAM 
#define LIBVNCSERVER_HAVE_GETSPNAM  1 
#endif

/* Define to 1 if you have the `gettimeofday' function. */
#ifndef LIBVNCSERVER_HAVE_GETTIMEOFDAY 
#define LIBVNCSERVER_HAVE_GETTIMEOFDAY  1 
#endif

/* Define to 1 if you have the `getuid' function. */
#ifndef LIBVNCSERVER_HAVE_GETUID 
#define LIBVNCSERVER_HAVE_GETUID  1 
#endif

/* Define to 1 if you have the `grantpt' function. */
#ifndef LIBVNCSERVER_HAVE_GRANTPT 
#define LIBVNCSERVER_HAVE_GRANTPT  1 
#endif

/* Define to 1 if you have the `inet_ntoa' function. */
#ifndef LIBVNCSERVER_HAVE_INET_NTOA 
#define LIBVNCSERVER_HAVE_INET_NTOA  1 
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef LIBVNCSERVER_HAVE_INTTYPES_H 
#define LIBVNCSERVER_HAVE_INTTYPES_H  1 
#endif

/* IRIX XReadDisplay available */
/* #undef LIBVNCSERVER_HAVE_IRIX_XREADDISPLAY */

/* Define to 1 if you have jpeglib.h header file. */
#ifndef LIBVNCSERVER_HAVE_JPEGLIB_H 
#define LIBVNCSERVER_HAVE_JPEGLIB_H  1 
#endif

/* libcrypt library present */
#ifndef LIBVNCSERVER_HAVE_LIBCRYPT 
#define LIBVNCSERVER_HAVE_LIBCRYPT  1 
#endif

/* Define to 1 if you have the `cygipc' library (-lcygipc). */
/* #undef LIBVNCSERVER_HAVE_LIBCYGIPC */

/* libjpeg present */
#ifndef LIBVNCSERVER_HAVE_LIBJPEG 
#define LIBVNCSERVER_HAVE_LIBJPEG  1 
#endif

/* Define to 1 if you have the `nsl' library (-lnsl). */
#ifndef LIBVNCSERVER_HAVE_LIBNSL 
#define LIBVNCSERVER_HAVE_LIBNSL  1 
#endif

/* Define to 1 if you have the `pthread' library (-lpthread). */
#ifndef LIBVNCSERVER_HAVE_LIBPTHREAD 
#define LIBVNCSERVER_HAVE_LIBPTHREAD  1 
#endif

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef LIBVNCSERVER_HAVE_LIBSOCKET */

/* openssl libssl present */
#ifndef LIBVNCSERVER_HAVE_LIBSSL 
#define LIBVNCSERVER_HAVE_LIBSSL  1 
#endif

/* XDAMAGE extension build environment present */
#ifndef LIBVNCSERVER_HAVE_LIBXDAMAGE 
#define LIBVNCSERVER_HAVE_LIBXDAMAGE  1 
#endif

/* XFIXES extension build environment present */
#ifndef LIBVNCSERVER_HAVE_LIBXFIXES 
#define LIBVNCSERVER_HAVE_LIBXFIXES  1 
#endif

/* XINERAMA extension build environment present */
#ifndef LIBVNCSERVER_HAVE_LIBXINERAMA 
#define LIBVNCSERVER_HAVE_LIBXINERAMA  1 
#endif

/* XRANDR extension build environment present */
#ifndef LIBVNCSERVER_HAVE_LIBXRANDR 
#define LIBVNCSERVER_HAVE_LIBXRANDR  1 
#endif

/* DEC-XTRAP extension build environment present */
/* #undef LIBVNCSERVER_HAVE_LIBXTRAP */

/* libz present */
#ifndef LIBVNCSERVER_HAVE_LIBZ 
#define LIBVNCSERVER_HAVE_LIBZ  1 
#endif

/* Define to 1 if you have the <limits.h> header file. */
#ifndef LIBVNCSERVER_HAVE_LIMITS_H 
#define LIBVNCSERVER_HAVE_LIMITS_H  1 
#endif

/* linux fb device build environment present */
#ifndef LIBVNCSERVER_HAVE_LINUX_FB_H 
#define LIBVNCSERVER_HAVE_LINUX_FB_H  1 
#endif

/* video4linux build environment present */
#ifndef LIBVNCSERVER_HAVE_LINUX_VIDEODEV_H 
#define LIBVNCSERVER_HAVE_LINUX_VIDEODEV_H  1 
#endif

/* Define to 1 if you have the `memmove' function. */
#ifndef LIBVNCSERVER_HAVE_MEMMOVE 
#define LIBVNCSERVER_HAVE_MEMMOVE  1 
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef LIBVNCSERVER_HAVE_MEMORY_H 
#define LIBVNCSERVER_HAVE_MEMORY_H  1 
#endif

/* Define to 1 if you have the `memset' function. */
#ifndef LIBVNCSERVER_HAVE_MEMSET 
#define LIBVNCSERVER_HAVE_MEMSET  1 
#endif

/* Define to 1 if you have the `mkfifo' function. */
#ifndef LIBVNCSERVER_HAVE_MKFIFO 
#define LIBVNCSERVER_HAVE_MKFIFO  1 
#endif

/* Define to 1 if you have the `mmap' function. */
#ifndef LIBVNCSERVER_HAVE_MMAP 
#define LIBVNCSERVER_HAVE_MMAP  1 
#endif

/* Define to 1 if you have the <netdb.h> header file. */
#ifndef LIBVNCSERVER_HAVE_NETDB_H 
#define LIBVNCSERVER_HAVE_NETDB_H  1 
#endif

/* Define to 1 if you have the <netinet/in.h> header file. */
#ifndef LIBVNCSERVER_HAVE_NETINET_IN_H 
#define LIBVNCSERVER_HAVE_NETINET_IN_H  1 
#endif

/* Define to 1 if you have the `pow' function. */
/* #undef LIBVNCSERVER_HAVE_POW */

/* Define to 1 if you have the `putenv' function. */
#ifndef LIBVNCSERVER_HAVE_PUTENV 
#define LIBVNCSERVER_HAVE_PUTENV  1 
#endif

/* Define to 1 if you have the <pwd.h> header file. */
#ifndef LIBVNCSERVER_HAVE_PWD_H 
#define LIBVNCSERVER_HAVE_PWD_H  1 
#endif

/* RECORD extension build environment present */
#ifndef LIBVNCSERVER_HAVE_RECORD 
#define LIBVNCSERVER_HAVE_RECORD  1 
#endif

/* Define to 1 if you have the `select' function. */
#ifndef LIBVNCSERVER_HAVE_SELECT 
#define LIBVNCSERVER_HAVE_SELECT  1 
#endif

/* Define to 1 if you have the `seteuid' function. */
#ifndef LIBVNCSERVER_HAVE_SETEUID 
#define LIBVNCSERVER_HAVE_SETEUID  1 
#endif

/* Define to 1 if you have the `setpgrp' function. */
#ifndef LIBVNCSERVER_HAVE_SETPGRP 
#define LIBVNCSERVER_HAVE_SETPGRP  1 
#endif

/* Define to 1 if you have the `setsid' function. */
#ifndef LIBVNCSERVER_HAVE_SETSID 
#define LIBVNCSERVER_HAVE_SETSID  1 
#endif

/* Define to 1 if you have the `setutxent' function. */
#ifndef LIBVNCSERVER_HAVE_SETUTXENT 
#define LIBVNCSERVER_HAVE_SETUTXENT  1 
#endif

/* Define to 1 if you have the `socket' function. */
#ifndef LIBVNCSERVER_HAVE_SOCKET 
#define LIBVNCSERVER_HAVE_SOCKET  1 
#endif

/* Solaris XReadScreen available */
/* #undef LIBVNCSERVER_HAVE_SOLARIS_XREADSCREEN */

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef LIBVNCSERVER_HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef LIBVNCSERVER_HAVE_STDINT_H 
#define LIBVNCSERVER_HAVE_STDINT_H  1 
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef LIBVNCSERVER_HAVE_STDLIB_H 
#define LIBVNCSERVER_HAVE_STDLIB_H  1 
#endif

/* Define to 1 if you have the `strchr' function. */
#ifndef LIBVNCSERVER_HAVE_STRCHR 
#define LIBVNCSERVER_HAVE_STRCHR  1 
#endif

/* Define to 1 if you have the `strdup' function. */
#ifndef LIBVNCSERVER_HAVE_STRDUP 
#define LIBVNCSERVER_HAVE_STRDUP  1 
#endif

/* Define to 1 if you have the `strerror' function. */
#ifndef LIBVNCSERVER_HAVE_STRERROR 
#define LIBVNCSERVER_HAVE_STRERROR  1 
#endif

/* Define to 1 if you have the `strftime' function. */
#ifndef LIBVNCSERVER_HAVE_STRFTIME 
#define LIBVNCSERVER_HAVE_STRFTIME  1 
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef LIBVNCSERVER_HAVE_STRINGS_H 
#define LIBVNCSERVER_HAVE_STRINGS_H  1 
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef LIBVNCSERVER_HAVE_STRING_H 
#define LIBVNCSERVER_HAVE_STRING_H  1 
#endif

/* Define to 1 if you have the `strpbrk' function. */
#ifndef LIBVNCSERVER_HAVE_STRPBRK 
#define LIBVNCSERVER_HAVE_STRPBRK  1 
#endif

/* Define to 1 if you have the `strrchr' function. */
#ifndef LIBVNCSERVER_HAVE_STRRCHR 
#define LIBVNCSERVER_HAVE_STRRCHR  1 
#endif

/* Define to 1 if you have the `strstr' function. */
#ifndef LIBVNCSERVER_HAVE_STRSTR 
#define LIBVNCSERVER_HAVE_STRSTR  1 
#endif

/* Define to 1 if you have the <syslog.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYSLOG_H 
#define LIBVNCSERVER_HAVE_SYSLOG_H  1 
#endif

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYS_IOCTL_H 
#define LIBVNCSERVER_HAVE_SYS_IOCTL_H  1 
#endif

/* Define to 1 if you have the <sys/socket.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYS_SOCKET_H 
#define LIBVNCSERVER_HAVE_SYS_SOCKET_H  1 
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYS_STAT_H 
#define LIBVNCSERVER_HAVE_SYS_STAT_H  1 
#endif

/* Define to 1 if you have the <sys/timeb.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYS_TIMEB_H 
#define LIBVNCSERVER_HAVE_SYS_TIMEB_H  1 
#endif

/* Define to 1 if you have the <sys/time.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYS_TIME_H 
#define LIBVNCSERVER_HAVE_SYS_TIME_H  1 
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef LIBVNCSERVER_HAVE_SYS_TYPES_H 
#define LIBVNCSERVER_HAVE_SYS_TYPES_H  1 
#endif

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#ifndef LIBVNCSERVER_HAVE_SYS_WAIT_H 
#define LIBVNCSERVER_HAVE_SYS_WAIT_H  1 
#endif

/* Define to 1 if you have the <time.h> header file. */
#ifndef LIBVNCSERVER_HAVE_TIME_H 
#define LIBVNCSERVER_HAVE_TIME_H  1 
#endif

/* Define to 1 if you have the `uname' function. */
#ifndef LIBVNCSERVER_HAVE_UNAME 
#define LIBVNCSERVER_HAVE_UNAME  1 
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef LIBVNCSERVER_HAVE_UNISTD_H 
#define LIBVNCSERVER_HAVE_UNISTD_H  1 
#endif

/* Define to 1 if you have the <utmpx.h> header file. */
#ifndef LIBVNCSERVER_HAVE_UTMPX_H 
#define LIBVNCSERVER_HAVE_UTMPX_H  1 
#endif

/* Define to 1 if you have the `vfork' function. */
#ifndef LIBVNCSERVER_HAVE_VFORK 
#define LIBVNCSERVER_HAVE_VFORK  1 
#endif

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef LIBVNCSERVER_HAVE_VFORK_H */

/* Define to 1 if you have the `vprintf' function. */
#ifndef LIBVNCSERVER_HAVE_VPRINTF 
#define LIBVNCSERVER_HAVE_VPRINTF  1 
#endif

/* Define to 1 if you have the `waitpid' function. */
#ifndef LIBVNCSERVER_HAVE_WAITPID 
#define LIBVNCSERVER_HAVE_WAITPID  1 
#endif

/* Define to 1 if `fork' works. */
#ifndef LIBVNCSERVER_HAVE_WORKING_FORK 
#define LIBVNCSERVER_HAVE_WORKING_FORK  1 
#endif

/* Define to 1 if `vfork' works. */
#ifndef LIBVNCSERVER_HAVE_WORKING_VFORK 
#define LIBVNCSERVER_HAVE_WORKING_VFORK  1 
#endif

/* X11 build environment present */
#ifndef LIBVNCSERVER_HAVE_X11 
#define LIBVNCSERVER_HAVE_X11  1 
#endif

/* XKEYBOARD extension build environment present */
#ifndef LIBVNCSERVER_HAVE_XKEYBOARD 
#define LIBVNCSERVER_HAVE_XKEYBOARD  1 
#endif

/* MIT-SHM extension build environment present */
#ifndef LIBVNCSERVER_HAVE_XSHM 
#define LIBVNCSERVER_HAVE_XSHM  1 
#endif

/* XTEST extension build environment present */
#ifndef LIBVNCSERVER_HAVE_XTEST 
#define LIBVNCSERVER_HAVE_XTEST  1 
#endif

/* XTEST extension has XTestGrabControl */
#ifndef LIBVNCSERVER_HAVE_XTESTGRABCONTROL 
#define LIBVNCSERVER_HAVE_XTESTGRABCONTROL  1 
#endif

/* Define to 1 if you have zlib.h header file. */
#ifndef LIBVNCSERVER_HAVE_ZLIB_H 
#define LIBVNCSERVER_HAVE_ZLIB_H  1 
#endif

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#ifndef LIBVNCSERVER_LSTAT_FOLLOWS_SLASHED_SYMLINK 
#define LIBVNCSERVER_LSTAT_FOLLOWS_SLASHED_SYMLINK  1 
#endif

/* Need a typedef for in_addr_t */
/* #undef LIBVNCSERVER_NEED_INADDR_T */

/* Name of package */
#ifndef LIBVNCSERVER_PACKAGE 
#define LIBVNCSERVER_PACKAGE  "italc" 
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef LIBVNCSERVER_PACKAGE_BUGREPORT 
#define LIBVNCSERVER_PACKAGE_BUGREPORT  "tobydox/at/users/dot/sf/dot/net" 
#endif

/* Define to the full name of this package. */
#ifndef LIBVNCSERVER_PACKAGE_NAME 
#define LIBVNCSERVER_PACKAGE_NAME  "italc" 
#endif

/* Define to the full name and version of this package. */
#ifndef LIBVNCSERVER_PACKAGE_STRING 
#define LIBVNCSERVER_PACKAGE_STRING  "italc 0.9.7.1-qt4-cvs-20060814" 
#endif

/* Define to the one symbol short name of this package. */
#ifndef LIBVNCSERVER_PACKAGE_TARNAME 
#define LIBVNCSERVER_PACKAGE_TARNAME  "italc" 
#endif

/* Define to the version of this package. */
#ifndef LIBVNCSERVER_PACKAGE_VERSION 
#define LIBVNCSERVER_PACKAGE_VERSION  "0.9.7.1-qt4-cvs-20060814" 
#endif

/* The number of bytes in type char */
/* #undef LIBVNCSERVER_SIZEOF_CHAR */

/* The number of bytes in type int */
/* #undef LIBVNCSERVER_SIZEOF_INT */

/* The number of bytes in type long */
/* #undef LIBVNCSERVER_SIZEOF_LONG */

/* The number of bytes in type short */
/* #undef LIBVNCSERVER_SIZEOF_SHORT */

/* The number of bytes in type void* */
/* #undef LIBVNCSERVER_SIZEOF_VOIDP */

/* Define to 1 if you have the ANSI C header files. */
#ifndef LIBVNCSERVER_STDC_HEADERS 
#define LIBVNCSERVER_STDC_HEADERS  1 
#endif

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#ifndef LIBVNCSERVER_TIME_WITH_SYS_TIME 
#define LIBVNCSERVER_TIME_WITH_SYS_TIME  1 
#endif

/* Version number of package */
#ifndef LIBVNCSERVER_VERSION 
#define LIBVNCSERVER_VERSION  "0.9.7.1-qt4-cvs-20060814" 
#endif

/* Defined if on Win32 platform */
/* #undef LIBVNCSERVER_WIN32 */

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef LIBVNCSERVER_WORDS_BIGENDIAN */

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef LIBVNCSERVER_X_DISPLAY_MISSING */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline__
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* The type for socklen */
/* #undef socklen_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */
 
/* once: _COMMON_INCLUDE_RFB_RFBCONFIG_H */
#endif
