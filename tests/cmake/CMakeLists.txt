# configure-time self-tests of the capability boundary check (cmake/modules/CheckCapabilityBoundary.cmake)

foreach(variant transitive interface direct plugin objects path option sources late allowed)
	add_test(NAME CapabilityBoundaryCheck-${variant}
		COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR}
			-S ${CMAKE_CURRENT_SOURCE_DIR}/capability-boundary
			-B ${CMAKE_CURRENT_BINARY_DIR}/capability-boundary-${variant}
			-DVARIANT=${variant} -DVEYON_SOURCE_DIR=${CMAKE_SOURCE_DIR})
	if(variant STREQUAL "allowed")
		set_tests_properties(CapabilityBoundaryCheck-${variant} PROPERTIES
			PASS_REGULAR_EXPRESSION "capability boundary check passed")
	else()
		set_tests_properties(CapabilityBoundaryCheck-${variant} PROPERTIES
			PASS_REGULAR_EXPRESSION "must not link veyon-crypto-authority")
	endif()
endforeach()

# the server ⇄ worker channel uses the authenticated local IPC only (02-ipc.md §6a): no loopback TCP
add_test(NAME WorkerChannelWithoutTcp
	COMMAND ${CMAKE_COMMAND} -DVEYON_SOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckWorkerChannelWithoutTcp.cmake)

# veyon-server is not D-Bus activatable, the capture helper owns no well-known bus name
add_test(NAME NoServerBusActivation
	COMMAND ${CMAKE_COMMAND} -DVEYON_SOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/CheckNoServerBusActivation.cmake)

# veyon-crypto is installed and packaged wherever veyon-core is (install rules only without WITH_CORE_ONLY)
if(NOT WITH_CORE_ONLY)
	add_test(NAME InstalledLibraries
		COMMAND ${CMAKE_COMMAND} -DVEYON_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DVEYON_BINARY_DIR=${CMAKE_BINARY_DIR}
			-P ${CMAKE_CURRENT_SOURCE_DIR}/CheckInstalledLibraries.cmake)
endif()
