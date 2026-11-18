# fails if veyon-crypto is not installed or packaged everywhere veyon-core is: veyon-core links it,
# so a package without it cannot start any Veyon binary
#
# - install: installs the crypto and core directories of the build tree (not the whole tree, whose
#   third-party targets may be unbuilt) into a scratch DESTDIR and requires the veyon-crypto
#   library next to every installed veyon-core library
# - Windows (windows-binaries) and Android (prepare-apk, androiddeployqt) copy or search the
#   libraries by build directory

# install
set(installDirectory ${VEYON_BINARY_DIR}/tests/cmake/installed-libraries)
file(REMOVE_RECURSE ${installDirectory})
foreach(directory crypto core)
	execute_process(COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${installDirectory} ${CMAKE_COMMAND} --install ${VEYON_BINARY_DIR}/${directory}
		RESULT_VARIABLE result OUTPUT_QUIET)
	if(NOT result EQUAL 0)
		message(FATAL_ERROR "installation of ${directory} into ${installDirectory} failed")
	endif()
endforeach()

file(GLOB_RECURSE installedFiles LIST_DIRECTORIES false RELATIVE ${installDirectory} ${installDirectory}/*)
set(coreLibraries "")
foreach(file ${installedFiles})
	get_filename_component(fileName ${file} NAME)
	if(fileName MATCHES "^(lib)?veyon-core\\.(so|dll|dylib)$")
		list(APPEND coreLibraries ${file})
	endif()
endforeach()
if(NOT coreLibraries)
	message(FATAL_ERROR "no veyon-core library installed")
endif()

foreach(coreLibrary ${coreLibraries})
	string(REPLACE "veyon-core" "veyon-crypto" cryptoLibrary ${coreLibrary})
	if(NOT EXISTS ${installDirectory}/${cryptoLibrary})
		message(FATAL_ERROR "${coreLibrary} is installed without ${cryptoLibrary}")
	endif()
endforeach()
file(REMOVE_RECURSE ${installDirectory})

# Windows and Android packaging: a line naming the core library or build directory must name the
# crypto one as well, or the same line for crypto must exist
foreach(packagingFile cmake/modules/WindowsInstaller.cmake CMakeLists.txt)
	file(READ ${VEYON_SOURCE_DIR}/${packagingFile} content)
	# lines contain semicolons (search paths), so split by hand
	string(REPLACE ";" "VEYON_SEMICOLON" content "${content}")
	string(REPLACE "\n" ";" lines "${content}")
	foreach(line ${lines})
		if(NOT line MATCHES "core/veyon-core\\.dll|CMAKE_BINARY_DIR}/core")
			continue()
		endif()
		string(REPLACE "core/veyon-core.dll" "crypto/veyon-crypto.dll" cryptoLine "${line}")
		string(REPLACE "CMAKE_BINARY_DIR}/core" "CMAKE_BINARY_DIR}/crypto" cryptoLine "${cryptoLine}")
		string(FIND "${line}" "CMAKE_BINARY_DIR}/crypto" inLine)
		string(FIND "${content}" "${cryptoLine}" inFile)
		if(inLine EQUAL -1 AND inFile EQUAL -1)
			string(REPLACE "VEYON_SEMICOLON" ";" line "${line}")
			message(FATAL_ERROR "${packagingFile}: veyon-crypto missing next to \"${line}\"")
		endif()
	endforeach()
endforeach()

message(STATUS "veyon-crypto installed and packaged with veyon-core (${coreLibraries})")
