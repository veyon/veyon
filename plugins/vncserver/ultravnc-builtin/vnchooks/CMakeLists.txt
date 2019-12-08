set(VH_WINRC "${CMAKE_CURRENT_BINARY_DIR}/vnchooksrc.obj")
add_custom_command(OUTPUT ${VH_WINRC}
						COMMAND ${WINDRES}
							-I${CMAKE_CURRENT_SOURCE_DIR}
							-o${VH_WINRC}
							-i${ultravnc_DIR}/winvnc/vnchooks/vnchooks.rc)

add_library(vnchooks MODULE
				${ultravnc_DIR}/winvnc/vnchooks/VNCHooks.cpp
				${ultravnc_DIR}/winvnc/vnchooks/SharedData.cpp
				${VH_WINRC})
target_compile_options(vnchooks PRIVATE ${VEYON_COMPILE_OPTIONS})
set_default_target_properties(vnchooks)
set_target_properties(vnchooks PROPERTIES PREFIX "")
set_target_properties(vnchooks PROPERTIES COMPILE_FLAGS "-Wno-write-strings -Wno-unused-variable -Wno-unknown-pragmas")
set_target_properties(vnchooks PROPERTIES LINK_FLAGS -Wl,-export-all-symbols)
target_link_libraries(vnchooks -ladvapi32)

