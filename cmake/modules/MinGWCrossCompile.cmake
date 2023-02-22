# this one is important
set(CMAKE_SYSTEM_NAME Windows)
#this one not so much
set(CMAKE_SYSTEM_VERSION 1)

set(MINGW_PREFIX /usr/${MINGW_TARGET}/)

# where is the target environment 
set(CMAKE_FIND_ROOT_PATH	${MINGW_PREFIX})
set(CMAKE_INSTALL_PREFIX	${MINGW_PREFIX})

set(MINGW_TOOL_PREFIX		/usr/bin/${MINGW_TARGET}-)

# specify the cross compiler
set(CMAKE_RC_COMPILER		${MINGW_TOOL_PREFIX}gcc)
set(CMAKE_C_COMPILER		${MINGW_TOOL_PREFIX}gcc)
set(CMAKE_CXX_COMPILER		${MINGW_TOOL_PREFIX}g++)

# specify location of some tools
set(STRIP					${MINGW_TOOL_PREFIX}strip)
set(WINDRES					${MINGW_TOOL_PREFIX}windres)

set(QT_BINARY_DIR			${MINGW_PREFIX}/bin)

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_INCLUDE_PATH ${MINGW_PREFIX}/include)
