# CheckCapabilityBoundary.cmake - Copyright (c) 2026 Tobias Junghans
#
# description: fail the configuration if a capability-restricted library is reachable from a
#              binary that is not explicitly allowed to use it (06-pki.md §1)
# usage: veyon_check_capability_boundary(LIBRARY <library> ALLOWED <targets...> [REQUIRED <targets...>])
#
# Every executable, shared library and module (plugins) in the build is checked, and its complete
# link closure is walked: LINK_LIBRARIES, INTERFACE_LINK_LIBRARIES(_DIRECT) and (INTERFACE_)
# LINK_OPTIONS of all reachable targets, recursively, with generator expressions such as
# $<LINK_ONLY:...> stripped. A binary violates the boundary if the library is in its closure, if
# a link item that is no target names the library (e.g. the path of its archive), or if a target
# in the closure compiles sources from the directories of the library's sources. REQUIRED targets
# must exist, so that the check cannot pass silently because a target was renamed.
#
# With CMake >= 3.19 the check runs at the end of the configuration of the top-level directory,
# so that it also sees targets and links added after the call; it prints
# "capability boundary check passed" on success.

function(_veyon_collect_buildsystem_targets directory outputVariable)
	get_property(targets DIRECTORY ${directory} PROPERTY BUILDSYSTEM_TARGETS)
	get_property(subdirectories DIRECTORY ${directory} PROPERTY SUBDIRECTORIES)
	foreach(subdirectory ${subdirectories})
		_veyon_collect_buildsystem_targets(${subdirectory} subdirectoryTargets)
		list(APPEND targets ${subdirectoryTargets})
	endforeach()
	set(${outputVariable} ${targets} PARENT_SCOPE)
endfunction()



function(_veyon_link_closure target library outputVariable)
	set(libraryNames ${library})
	get_target_property(libraryOutputName ${library} OUTPUT_NAME)
	if(libraryOutputName)
		list(APPEND libraryNames ${libraryOutputName})
	endif()
	set(queue ${target})
	set(closure "")
	while(queue)
		list(POP_FRONT queue current)
		if(current IN_LIST closure OR NOT TARGET ${current})
			continue()
		endif()
		list(APPEND closure ${current})

		get_target_property(aliasedTarget ${current} ALIASED_TARGET)
		if(aliasedTarget)
			list(APPEND queue ${aliasedTarget})
			continue()
		endif()

		get_target_property(type ${current} TYPE)
		get_target_property(imported ${current} IMPORTED)
		set(properties INTERFACE_LINK_LIBRARIES INTERFACE_LINK_LIBRARIES_DIRECT INTERFACE_LINK_OPTIONS)
		if(NOT type STREQUAL "INTERFACE_LIBRARY" AND NOT imported)
			list(APPEND properties LINK_LIBRARIES LINK_OPTIONS)
		endif()

		foreach(property ${properties})
			get_target_property(dependencies ${current} ${property})
			if(NOT dependencies)
				continue()
			endif()
			foreach(dependency ${dependencies})
				# conservative: every target named anywhere in a (possibly nested) generator
				# expression such as $<LINK_ONLY:x> or $<$<CONFIG:Debug>:x> counts as a dependency;
				# "::" (namespaced targets) is kept, the directory markers "::@..." are skipped
				string(REPLACE "::" "VEYON_DOUBLE_COLON" dependency "${dependency}")
				string(REGEX REPLACE "[$<>:,]" ";" dependency "${dependency}")
				string(REPLACE "VEYON_DOUBLE_COLON" "::" dependency "${dependency}")
				foreach(name ${dependency})
					if(name MATCHES "^::@")
						continue()
					elseif(TARGET ${name})
						list(APPEND queue ${name})
						continue()
					endif()
					# raw path or linker flag naming the library's archive
					foreach(libraryName ${libraryNames})
						string(FIND "${name}" "${libraryName}" position)
						if(position GREATER_EQUAL 0)
							list(APPEND closure ${library})
						endif()
					endforeach()
				endforeach()
			endforeach()
		endforeach()
	endwhile()
	set(${outputVariable} ${closure} PARENT_SCOPE)
endfunction()



# absolute paths of the (non-generated) sources of a target
function(_veyon_target_source_files target outputVariable)
	get_target_property(sourceDirectory ${target} SOURCE_DIR)
	get_target_property(sources ${target} SOURCES)
	set(files "")
	foreach(source ${sources})
		if(NOT source MATCHES "\\$<")
			get_filename_component(file "${source}" ABSOLUTE BASE_DIR "${sourceDirectory}")
			list(APPEND files "${file}")
		endif()
	endforeach()
	set(${outputVariable} ${files} PARENT_SCOPE)
endfunction()



function(_veyon_check_capability_boundary)
	cmake_parse_arguments(arg "" "LIBRARY" "ALLOWED;REQUIRED" ${ARGN})

	foreach(requiredTarget ${arg_REQUIRED} ${arg_LIBRARY})
		if(NOT TARGET ${requiredTarget})
			message(FATAL_ERROR "capability boundary check: required target ${requiredTarget} does not exist")
		endif()
	endforeach()

	_veyon_target_source_files(${arg_LIBRARY} librarySources)
	set(librarySourceDirectories "")
	foreach(file ${librarySources})
		get_filename_component(directory "${file}" DIRECTORY)
		list(APPEND librarySourceDirectories "${directory}/")
	endforeach()
	list(REMOVE_DUPLICATES librarySourceDirectories)

	_veyon_collect_buildsystem_targets(${CMAKE_SOURCE_DIR} targets)
	foreach(target ${targets})
		get_target_property(type ${target} TYPE)
		if(NOT type MATCHES "^(EXECUTABLE|SHARED_LIBRARY|MODULE_LIBRARY)$" OR target IN_LIST arg_ALLOWED)
			continue()
		endif()
		_veyon_link_closure(${target} ${arg_LIBRARY} closure)
		if(arg_LIBRARY IN_LIST closure)
			message(FATAL_ERROR "${target} must not link ${arg_LIBRARY} (capability boundary, 06-pki.md §1)")
		endif()
		foreach(dependency ${closure})
			get_target_property(imported ${dependency} IMPORTED)
			get_target_property(dependencyType ${dependency} TYPE)
			if(imported OR dependencyType STREQUAL "INTERFACE_LIBRARY")
				continue()
			endif()
			_veyon_target_source_files(${dependency} sources)
			foreach(source ${sources})
				foreach(directory ${librarySourceDirectories})
					string(FIND "${source}" "${directory}" position)
					if(position EQUAL 0)
						message(FATAL_ERROR "${target} must not link ${arg_LIBRARY} (capability boundary, 06-pki.md §1): "
							"${dependency} compiles ${source}")
					endif()
				endforeach()
			endforeach()
		endforeach()
	endforeach()

	message(STATUS "capability boundary check passed")
endfunction()



function(_veyon_check_capability_boundary_deferred)
	get_property(arguments GLOBAL PROPERTY VEYON_CAPABILITY_BOUNDARY_CHECK_ARGUMENTS)
	_veyon_check_capability_boundary(${arguments})
endfunction()



function(veyon_check_capability_boundary)
	if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
		# run once all targets and links of the project exist
		set_property(GLOBAL PROPERTY VEYON_CAPABILITY_BOUNDARY_CHECK_ARGUMENTS ${ARGN})
		cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _veyon_check_capability_boundary_deferred)
	else()
		_veyon_check_capability_boundary(${ARGN})
	endif()
endfunction()
