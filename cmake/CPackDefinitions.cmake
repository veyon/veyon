#
# generate packages
#
# Environment
if (NOT CPACK_SYSTEM_NAME)
        set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_PROCESSOR}")
endif ()


# Basic information
if(NOT CPACK_PACKAGE_NAME)
	set(CPACK_PACKAGE_NAME "veyon")
endif()
SET(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")

SET(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_SYSTEM_NAME}")
SET(CPACK_PACKAGE_CONTACT "Tobias Junghans <tobydox@veyon.io>")
SET(CPACK_PACKAGE_HOMEPAGE "https://veyon.io")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open source computer monitoring and classroom management")
SET(CPACK_PACKAGE_VENDOR "Veyon Solutions")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY TRUE)
SET(CPACK_SOURCE_IGNORE_FILES "${CMAKE_SOURCE_DIR}/build/;${CMAKE_SOURCE_DIR}/.git/;")
SET(CPACK_STRIP_FILES  TRUE)

# DEB package
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Open source computer monitoring and classroom management software
  Veyon is a free and open source software for computer monitoring and classroom
  management supporting Windows and Linux. It enables teachers to view and control
  computer labs and interact with students. Veyon is available in many different
  languages and provides numerous features supporting teachers and administrators
  at their daily work:
  .
  * Overview: monitor all computers in one or multiple locations or classrooms
  * Remote access: view or control computers to watch and support users
  * Demo: broadcast the teacher's screen in realtime (fullscreen/window)
  * Screen lock: draw attention to what matters right now
  * Communication: send text messages to students
  * Start and end lessons: log in and log out users all at once
  * Screenshots: record learning progress and document infringements
  * Programs & websites: launch programs and open website URLs remotely
  * Teaching material: distribute and open documents, images and videos easily
  * Administration: power on/off and reboot computers remotely")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Education")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libqca-qt5-2-plugins")
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_DEBIAN_COMPRESSION_TYPE "xz")

FUNCTION(ReadRelease valuename FROM filename INTO varname)
  file (STRINGS ${filename} _distrib
    REGEX "^${valuename}="
    )
  string (REGEX REPLACE
    "^${valuename}=\"?\(.*\)" "\\1" ${varname} "${_distrib}"
    )
  # remove trailing quote that got globbed by the wildcard (greedy match)
  string (REGEX REPLACE
    "\"$" "" ${varname} "${${varname}}"
    )
  set (${varname} "${${varname}}" PARENT_SCOPE)
ENDFUNCTION()

# RPM package
IF(EXISTS /etc/os-release)
ReadRelease("NAME" FROM /etc/os-release INTO OS_NAME)
IF(OS_NAME MATCHES ".*openSUSE.*")
	SET(OS_OPENSUSE TRUE)
ENDIF()
ENDIF()

IF(OS_OPENSUSE)
SET(CPACK_RPM_PACKAGE_REQUIRES ${CPACK_RPM_PACKAGE_REQUIRES} "libqca-qt5-plugins")
ELSE()
SET(CPACK_RPM_PACKAGE_REQUIRES ${CPACK_RPM_PACKAGE_REQUIRES} "qca-qt5-ossl")
ENDIF()
SET(CPACK_RPM_PACKAGE_LICENSE "GPLv2")
SET(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_DEBIAN_PACKAGE_DESCRIPTION})
SET(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /lib)


# Generators
IF (WIN32)    # TODO
    IF (USE_WIX_TOOLSET)
        SET(CPACK_GENERATOR "WIX") # this need WiX Tooset installed and a path to candle.exe
    ELSE ()
        SET(CPACK_GENERATOR "NSIS") # this needs NSIS installed, and available
    ENDIF ()
    SET(CPACK_SOURCE_GENERATOR "ZIP")
ELSEIF ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")   # TODO
     SET(CPACK_GENERATOR "PackageMake")
ELSE ()
     IF(EXISTS /etc/redhat-release OR EXISTS /etc/fedora-release OR OS_OPENSUSE)
        SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}.${CPACK_SYSTEM_NAME}")
        SET(CPACK_GENERATOR "RPM")
     ENDIF ()
     IF(EXISTS /etc/debian_version)
        if (CPACK_SYSTEM_NAME STREQUAL "x86_64")
                set(CPACK_SYSTEM_NAME "amd64")
        endif ()
        SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_SYSTEM_NAME}")
        SET(CPACK_GENERATOR "DEB")
     ENDIF ()
     SET(CPACK_SOURCE_GENERATOR "TGZ")
ENDIF ()


INCLUDE(CPack)
# To generate packages use:
# make package
# make package_source

