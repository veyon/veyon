#.rst:
# FindLibVNCClient
# --------
#
# Try to find the LibVNCClient library, once done this will define:
#
# ``LibVNCClient_FOUND``
#	 System has LibVNCClient.
#
# ``LibVNCClient_INCLUDE_DIRS``
#	 The LibVNCClient include directory.
#
# ``LibVNCClient_LIBRARIES``
#	 The LibVNCClient libraries.
#
# ``LibVNCClient_VERSION``
#	 The LibVNCClient version.
#
# If ``LibVNCClient_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``LibVNC::LibVNCClient``
#	 The LibVNCClient library

#=============================================================================
# SPDX-FileCopyrightText: 2020-2021 Tobias Junghans <tobydox@veyon.io>
#
# SPDX-License-Identifier: BSD-3-Clause
#=============================================================================

find_package(PkgConfig QUIET)
pkg_check_modules(PC_LIBVNCCLIENT QUIET libvncclient)

find_path(LibVNCClient_INCLUDE_DIRS NAMES rfb/rfbclient.h HINTS ${PC_LIBVNCCLIENT_INCLUDE_DIRS})
find_library(LibVNCClient_LIBRARIES NAMES vncclient HINTS ${PC_LIBVNCCLIENT_LIBRARY_DIRS})

set(LibVNCClient_VERSION ${PC_LIBVNCCLIENT_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibVNCClient
	FOUND_VAR LibVNCClient_FOUND
	REQUIRED_VARS LibVNCClient_INCLUDE_DIRS LibVNCClient_LIBRARIES
	VERSION_VAR LibVNCClient_VERSION
)

mark_as_advanced(LibVNCClient_INCLUDE_DIRS LibVNCClient_LIBRARIES)

if(LibVNCClient_FOUND AND NOT TARGET LibVNCClient::LibVNCClient)
	add_library(LibVNC::LibVNCClient UNKNOWN IMPORTED)
	set_target_properties(LibVNC::LibVNCClient PROPERTIES
		IMPORTED_LOCATION "${LibVNCClient_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${LibVNCClient_INCLUDE_DIRS}"
		INTERFACE_COMPILE_DEFINITIONS "${PC_LIBVNCCLIENT_CFLAGS_OTHER}"
	)
endif()

include(FeatureSummary)
set_package_properties(LibVNCClient PROPERTIES
	DESCRIPTION "cross-platform C library which allows you to easily implement VNC client functionality"
	URL "https://libvnc.github.io/"
)
