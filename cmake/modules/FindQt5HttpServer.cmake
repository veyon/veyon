# Minimal finder for Qt5HttpServer that allows falling back to the bundled
# qthttpserver sources. We deliberately do not raise a fatal error here since
# the build can proceed by compiling the copy shipped in 3rdparty/qthttpserver.

set(Qt5HttpServer_FOUND FALSE)
set(Qt5HttpServer_VERSION "0.0.0")

if(NOT Qt5HttpServer_FIND_QUIETLY)
  message(STATUS "Qt5HttpServer not found in system packages - will use bundled qthttpserver sources")
endif()

unset(Qt5HttpServer_DIR CACHE)
unset(Qt5HttpServer_LIBRARIES CACHE)
