#.rst:
# Findgbm
# -------
#
# Try to find gbm on a Unix system.
#
# This will define the following variables:
#
# ``gbm_FOUND``
#     True if (the requested version of) gbm is available
# ``gbm_VERSION``
#     The version of gbm
# ``gbm_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the ``gbm::gbm``
#     target
# ``gbm_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the target is not
#     used for linking
# ``gbm_DEFINITIONS``
#     This should be passed to target_compile_options() if the target is not
#     used for linking
#
# If ``gbm_FOUND`` is TRUE, it will also define the following imported target:
#
# ``gbm::gbm``
#     The gbm library
#
# In general we recommend using the imported target, as it is easier to use.
# Bear in mind, however, that if the target is in the link interface of an
# exported library, it must be made available by the package config file.

#=============================================================================
# SPDX-FileCopyrightText: 2014 Alex Merry <alex.merry@kde.org>
# SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause
#=============================================================================

if(CMAKE_VERSION VERSION_LESS 2.8.12)
    message(FATAL_ERROR "CMake 2.8.12 is required by Findgbm.cmake")
endif()
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
    message(AUTHOR_WARNING "Your project should require at least CMake 2.8.12 to use Findgbm.cmake")
endif()

if(NOT WIN32)
    # Use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PKG_gbm QUIET gbm)

    set(gbm_DEFINITIONS ${PKG_gbm_CFLAGS_OTHER})
    set(gbm_VERSION ${PKG_gbm_VERSION})

    find_path(gbm_INCLUDE_DIR
        NAMES
            gbm.h
        HINTS
            ${PKG_gbm_INCLUDE_DIRS}
    )
    find_library(gbm_LIBRARY
        NAMES
            gbm
        HINTS
            ${PKG_gbm_LIBRARY_DIRS}
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(gbm
        FOUND_VAR
            gbm_FOUND
        REQUIRED_VARS
            gbm_LIBRARY
            gbm_INCLUDE_DIR
        VERSION_VAR
            gbm_VERSION
    )

    if(gbm_FOUND AND NOT TARGET gbm::gbm)
        add_library(gbm::gbm UNKNOWN IMPORTED)
        set_target_properties(gbm::gbm PROPERTIES
            IMPORTED_LOCATION "${gbm_LIBRARY}"
            INTERFACE_COMPILE_OPTIONS "${gbm_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${gbm_INCLUDE_DIR}"
        )
    endif()

    mark_as_advanced(gbm_LIBRARY gbm_INCLUDE_DIR)

    # compatibility variables
    set(gbm_LIBRARIES ${gbm_LIBRARY})
    set(gbm_INCLUDE_DIRS ${gbm_INCLUDE_DIR})
    set(gbm_VERSION_STRING ${gbm_VERSION})

else()
    message(STATUS "Findgbm.cmake cannot find gbm on Windows systems.")
    set(gbm_FOUND FALSE)
endif()

include(FeatureSummary)
set_package_properties(gbm PROPERTIES
    URL "https://www.mesa3d.org"
    DESCRIPTION "Mesa gbm library."
)
