#.rst:
# FindLibVNCServer
# --------
#
# Try to find the LibVNCServer library, once done this will define:
#
# ``LibVNCServer_FOUND``
#	 System has LibVNCServer.
#
# ``LibVNCServer_INCLUDE_DIRS``
#	 The LibVNCServer include directory.
#
# ``LibVNCServer_LIBRARIES``
#	 The LibVNCServer libraries.
#
# ``LibVNCServer_VERSION``
#	 The LibVNCServer version.
#
# If ``LibVNCServer_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``LibVNC::LibVNCServer``
#	 The LibVNCServer library

#=============================================================================
# SPDX-FileCopyrightText: 2020-2021 Tobias Junghans <tobydox@veyon.io>
#
# SPDX-License-Identifier: BSD-3-Clause
#=============================================================================

find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBVNCCLIENT QUIET libvncserver)

find_path(LibVNCServer_INCLUDE_DIRS NAMES rfb/rfb.h HINTS ${PC_LIBVNCCLIENT_INCLUDE_DIRS})
find_library(LibVNCServer_LIBRARIES NAMES vncserver HINTS ${PC_LIBVNCCLIENT_LIBRARY_DIRS})

set(LibVNCServer_VERSION ${PC_LIBVNCCLIENT_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibVNCServer
	FOUND_VAR LibVNCServer_FOUND
	REQUIRED_VARS LibVNCServer_INCLUDE_DIRS LibVNCServer_LIBRARIES
	VERSION_VAR LibVNCServer_VERSION
)

mark_as_advanced(LibVNCServer_INCLUDE_DIRS LibVNCServer_LIBRARIES)

if(LibVNCServer_FOUND AND NOT TARGET LibVNCServer::LibVNCServer)
	add_library(LibVNC::LibVNCServer UNKNOWN IMPORTED)
	set_target_properties(LibVNC::LibVNCServer PROPERTIES
		IMPORTED_LOCATION "${LibVNCServer_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${LibVNCServer_INCLUDE_DIRS}"
		INTERFACE_COMPILE_DEFINITIONS "${PC_LIBVNCCLIENT_CFLAGS_OTHER}"
	)
endif()

include(FeatureSummary)
set_package_properties(LibVNCServer PROPERTIES
	DESCRIPTION "cross-platform C library which allows you to easily implement VNC server functionality"
	URL "https://libvnc.github.io/"
)
