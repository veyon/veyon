#.rst:
# FindSasl2
# ---------
#
# Try to find the SASL2 library.
#
# This will define the following variables:
#
# ``Sasl2_FOUND``
#     System has SASL2.
#
# ``Sasl2_VERSION``
#     The version of SASL2.
#
# ``Sasl2_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if
#     the target is not used for linking.
#
# ``Sasl2_LIBRARIES``
#     The SASL2 library.
#     This can be passed to target_link_libraries() instead of
#     the ``Sasl2::Sasl2`` target
#
# If ``Sasl2_FOUND`` is TRUE, the following imported target
# will be available:
#
# ``Sasl2::Sasl2``
#     The SASL2 library
#
# Since 5.41.0.
#
#=============================================================================
# Copyright 2006, 2007 Laurent Montel <montel@kde.org>
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

# NOTE: libsasl2.pc doesn't export the include dir.
find_package(PkgConfig QUIET)
pkg_check_modules(PC_Sasl2 libsasl2)

find_path(Sasl2_INCLUDE_DIRS NAMES sasl/sasl.h)

# libsasl2 add for windows, because the windows package of cyrus-sasl2
# contains a libsasl2 also for msvc which is not standard conform
find_library(Sasl2_LIBRARIES
  NAMES sasl2 libsasl2
  HINTS ${PC_Sasl2_LIBRARY_DIRS}
)

set(Sasl2_VERSION "${PC_Sasl2_VERSION}")

if(NOT Sasl2_VERSION)
  if(EXISTS "${Sasl2_INCLUDE_DIRS}/sasl/sasl.h")
    file(READ "${Sasl2_INCLUDE_DIRS}/sasl/sasl.h" SASL2_H_CONTENT)
    string(REGEX MATCH "#define SASL_VERSION_MAJOR[ ]+[0-9]+" SASL2_VERSION_MAJOR_MATCH ${SASL2_H_CONTENT})
    string(REGEX MATCH "#define SASL_VERSION_MINOR[ ]+[0-9]+" SASL2_VERSION_MINOR_MATCH ${SASL2_H_CONTENT})
    string(REGEX MATCH "#define SASL_VERSION_STEP[ ]+[0-9]+" SASL2_VERSION_STEP_MATCH ${SASL2_H_CONTENT})

    string(REGEX REPLACE ".*_MAJOR[ ]+(.*)" "\\1" SASL2_VERSION_MAJOR ${SASL2_VERSION_MAJOR_MATCH})
    string(REGEX REPLACE ".*_MINOR[ ]+(.*)" "\\1" SASL2_VERSION_MINOR ${SASL2_VERSION_MINOR_MATCH})
    string(REGEX REPLACE ".*_STEP[ ]+(.*)" "\\1"  SASL2_VERSION_STEP  ${SASL2_VERSION_STEP_MATCH})

    set(Sasl2_VERSION "${SASL2_VERSION_MAJOR}.${SASL2_VERSION_MINOR}.${SASL2_VERSION_STEP}")
  else()
  # Could not find the version
  set(Sasl2_VERSION "0.0.0")
  endif()
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sasl2
    FOUND_VAR Sasl2_FOUND
    REQUIRED_VARS Sasl2_LIBRARIES Sasl2_INCLUDE_DIRS
    VERSION_VAR Sasl2_VERSION
)
if(Sasl2_FOUND AND NOT TARGET Sasl2::Sasl2)
  add_library(Sasl2::Sasl2 UNKNOWN IMPORTED)
  set_target_properties(Sasl2::Sasl2 PROPERTIES
  IMPORTED_LOCATION "${Sasl2_LIBRARIES}"
  INTERFACE_INCLUDE_DIRECTORIES "${Sasl2_INCLUDE_DIRS}")
endif()

mark_as_advanced(Sasl2_LIBRARIES Sasl2_INCLUDE_DIRS Sasl2_VERSION)

include(FeatureSummary)
set_package_properties(Sasl2 PROPERTIES
  URL "https://www.cyrusimap.org/sasl/"
  DESCRIPTION "The Cyrus-sasl library."
)

