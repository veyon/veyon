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
