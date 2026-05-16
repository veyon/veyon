# FindPipeWire.cmake - find PipeWire library
#
# Copyright (c) 2024-2026 Tobias Junghans <tobydox@veyon.io>
#
# This file is part of Veyon - https://veyon.io
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This module defines:
#   PipeWire_FOUND        - system has PipeWire
#   PipeWire_INCLUDE_DIRS - include directories
#   PipeWire_LIBRARIES    - libraries to link against
#   PipeWire_VERSION      - version string
#   PipeWire::PipeWire    - imported target

find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_PIPEWIRE QUIET libpipewire-0.3)
endif()

find_path(PipeWire_INCLUDE_DIR
	NAMES pipewire/pipewire.h
	HINTS ${PC_PIPEWIRE_INCLUDE_DIRS}
	PATH_SUFFIXES pipewire-0.3
)

find_library(PipeWire_LIBRARY
	NAMES pipewire-0.3
	HINTS ${PC_PIPEWIRE_LIBRARY_DIRS}
)

# Also look for spa headers which come with pipewire
find_path(Spa_INCLUDE_DIR
	NAMES spa/param/video/format-utils.h
	HINTS ${PC_PIPEWIRE_INCLUDE_DIRS}
	PATH_SUFFIXES spa-0.2
)

set(PipeWire_VERSION ${PC_PIPEWIRE_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PipeWire
	REQUIRED_VARS PipeWire_LIBRARY PipeWire_INCLUDE_DIR Spa_INCLUDE_DIR
	VERSION_VAR PipeWire_VERSION
)

if(PipeWire_FOUND)
	set(PipeWire_INCLUDE_DIRS ${PipeWire_INCLUDE_DIR} ${Spa_INCLUDE_DIR})
	set(PipeWire_LIBRARIES ${PipeWire_LIBRARY})

	if(NOT TARGET PipeWire::PipeWire)
		add_library(PipeWire::PipeWire UNKNOWN IMPORTED)
		set_target_properties(PipeWire::PipeWire PROPERTIES
			IMPORTED_LOCATION "${PipeWire_LIBRARY}"
			INTERFACE_INCLUDE_DIRECTORIES "${PipeWire_INCLUDE_DIRS}"
		)
	endif()
endif()

mark_as_advanced(PipeWire_INCLUDE_DIR PipeWire_LIBRARY Spa_INCLUDE_DIR)
