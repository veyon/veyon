SET(CMAKE_MODULE_LINKER_FLAGS -Wl,-export-all-symbols)

LINK_LIBRARIES(-ladvapi32)

SET(VH_WINRC "${CMAKE_CURRENT_BINARY_DIR}/vnchooksrc.obj")
ADD_CUSTOM_COMMAND(OUTPUT ${VH_WINRC}
						COMMAND ${WINDRES}
							-I${CMAKE_CURRENT_SOURCE_DIR}
							-o${VH_WINRC}
							-i${ultravnc_DIR}/winvnc/vnchooks/vnchooks.rc)

ADD_LIBRARY(vnchooks MODULE
				${ultravnc_DIR}/winvnc/vnchooks/VNCHooks.cpp
				${ultravnc_DIR}/winvnc/vnchooks/SharedData.cpp
				${VH_WINRC})
SET_TARGET_PROPERTIES(vnchooks PROPERTIES PREFIX "")

