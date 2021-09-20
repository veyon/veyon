# Find liblzo2
# LZO_FOUND - system has the LZO library
# LZO_INCLUDE_DIR - the LZO include directory
# LZO_LIBRARIES - The libraries needed to use LZO

if(LZO_INCLUDE_DIR AND LZO_LIBRARIES)
	# in cache already
	set(LZO_FOUND TRUE)
else()
	find_path(LZO_INCLUDE_DIR NAMES lzo/lzo1x.h)

	find_library(LZO_LIBRARIES NAMES lzo2)

	if(LZO_INCLUDE_DIR AND LZO_LIBRARIES)
		 set(LZO_FOUND TRUE)
	endif()

	if(LZO_FOUND)
		 if(NOT LZO_FIND_QUIETLY)
				message(STATUS "Found LZO: ${LZO_LIBRARIES}")
		 endif()
	else()
		 if(LZO_FIND_REQUIRED)
				message(FATAL_ERROR "Could NOT find LZO")
		 endif()
	endif()

	mark_as_advanced(LZO_INCLUDE_DIR LZO_LIBRARIES)
endif()
