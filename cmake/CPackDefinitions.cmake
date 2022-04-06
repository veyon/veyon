# Basic information
if(NOT CPACK_PACKAGE_NAME)
	set(CPACK_PACKAGE_NAME "veyon")
endif()
set(CPACK_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_CONTACT "Tobias Junghans <tobydox@veyon.io>")
set(CPACK_PACKAGE_HOMEPAGE "https://veyon.io")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform computer control and classroom management")
set(CPACK_PACKAGE_VENDOR "Veyon Solutions")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY TRUE)
set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_SOURCE_DIR}/build/;${CMAKE_SOURCE_DIR}/.git/;")
set(CPACK_STRIP_FILES  TRUE)

# DEB package
if(${CMAKE_VERSION} VERSION_GREATER "3.15.0")
	set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"Veyon is a free and open source software for monitoring and controlling
computers across multiple platforms. Veyon supports you in teaching in digital
learning environments, performing virtual trainings or giving remote support.

The following features are available in Veyon:

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
endif()

set(CPACK_DEBIAN_PACKAGE_SECTION "Education")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqca-qt5-2-plugins")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")

function(ReadRelease valuename FROM filename INTO varname)
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
endfunction()

# RPM package
if(EXISTS /etc/os-release)
	ReadRelease("NAME" FROM /etc/os-release INTO OS_NAME)
	if(OS_NAME MATCHES ".*openSUSE.*")
		set(OS_OPENSUSE TRUE)
	endif()
endif()

if(OS_OPENSUSE)
	if(WITH_QT6)
		set(CPACK_RPM_PACKAGE_REQUIRES ${CPACK_RPM_PACKAGE_REQUIRES} "qca-qt6-plugins")
	else()
		set(CPACK_RPM_PACKAGE_REQUIRES ${CPACK_RPM_PACKAGE_REQUIRES} "libqca-qt5-plugins")
	endif()
else()
	set(CPACK_RPM_PACKAGE_REQUIRES ${CPACK_RPM_PACKAGE_REQUIRES} "qca-qt5-ossl")
endif()
set(CPACK_RPM_PACKAGE_LICENSE "GPLv2")
set(CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_DEBIAN_PACKAGE_DESCRIPTION})
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION /lib)


# Generators
if(WIN32)    # TODO
	if(USE_WIX_TOOLSET)
		set(CPACK_GENERATOR "WIX") # this need WiX Tooset installed and a path to candle.exe
	else()
		set(CPACK_GENERATOR "NSIS") # this needs NSIS installed, and available
	endif()
	set(CPACK_SOURCE_GENERATOR "ZIP")
elseif( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")   # TODO
	set(CPACK_GENERATOR "PackageMake")
else()
	if(EXISTS /etc/redhat-release OR EXISTS /etc/fedora-release OR OS_OPENSUSE)
		set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
		set(CPACK_RPM_PACKAGE_RELEASE "${CPACK_DIST}")
		set(CPACK_GENERATOR "RPM")
	endif()
	if(EXISTS /etc/debian_version)
		set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
		set(CPACK_DEBIAN_PACKAGE_RELEASE "${CPACK_DIST}")
		set(CPACK_GENERATOR "DEB")
	endif()
	set(CPACK_SOURCE_GENERATOR "TGZ")
endif()

