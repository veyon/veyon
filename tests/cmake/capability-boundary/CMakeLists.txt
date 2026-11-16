# Fixture for the capability boundary check: configured by the CapabilityBoundaryCheck-* tests in
# tests/cmake/CMakeLists.txt with VARIANT=<variant>, see there

cmake_minimum_required(VERSION 3.18.0)
project(capability-boundary-fixture C)

include(${VEYON_SOURCE_DIR}/cmake/modules/CheckCapabilityBoundary.cmake)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/authority/authority.c "int authority(void) { return 0; }\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/helper.c "int helper(void) { return 0; }\n")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/main.c "int main(void) { return 0; }\n")

add_library(veyon-crypto-authority STATIC ${CMAKE_CURRENT_BINARY_DIR}/authority/authority.c)

# static helper: its PRIVATE dependency ends up as $<LINK_ONLY:...> in INTERFACE_LINK_LIBRARIES
add_library(helper STATIC ${CMAKE_CURRENT_BINARY_DIR}/helper.c)
target_link_libraries(helper PRIVATE veyon-crypto-authority)

add_executable(veyon-master ${CMAKE_CURRENT_BINARY_DIR}/main.c)

if(VARIANT STREQUAL "transitive")
	target_link_libraries(veyon-master PRIVATE helper)
elseif(VARIANT STREQUAL "interface")
	add_library(bundle INTERFACE)
	target_link_libraries(bundle INTERFACE $<$<BOOL:1>:veyon-crypto-authority>)
	add_executable(veyon-worker ${CMAKE_CURRENT_BINARY_DIR}/main.c)
	target_link_libraries(veyon-worker PRIVATE bundle)
elseif(VARIANT STREQUAL "direct")
	add_library(bundle INTERFACE)
	set_property(TARGET bundle PROPERTY INTERFACE_LINK_LIBRARIES_DIRECT veyon-crypto-authority)
	target_link_libraries(veyon-master PRIVATE bundle)
elseif(VARIANT STREQUAL "plugin")
	add_library(someplugin MODULE ${CMAKE_CURRENT_BINARY_DIR}/helper.c)
	target_link_libraries(someplugin PRIVATE helper)
elseif(VARIANT STREQUAL "objects")
	add_library(objects OBJECT ${CMAKE_CURRENT_BINARY_DIR}/helper.c)
	target_link_libraries(objects PRIVATE veyon-crypto-authority)
	target_link_libraries(veyon-master PRIVATE objects)
elseif(VARIANT STREQUAL "path")
	target_link_libraries(veyon-master PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/libveyon-crypto-authority.a)
elseif(VARIANT STREQUAL "option")
	target_link_options(veyon-master PRIVATE $<TARGET_FILE:veyon-crypto-authority>)
elseif(VARIANT STREQUAL "sources")
	get_target_property(authoritySources veyon-crypto-authority SOURCES)
	target_sources(veyon-master PRIVATE ${authoritySources})
elseif(VARIANT STREQUAL "late" OR VARIANT STREQUAL "allowed")
	add_executable(veyon-service ${CMAKE_CURRENT_BINARY_DIR}/main.c)
	target_link_libraries(veyon-service PRIVATE helper)
else()
	message(FATAL_ERROR "unknown VARIANT")
endif()

veyon_check_capability_boundary(LIBRARY veyon-crypto-authority ALLOWED veyon-service REQUIRED veyon-master)

# links added after the check must be checked as well
if(VARIANT STREQUAL "late")
	target_link_libraries(veyon-master PRIVATE helper)
endif()
