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
SET_TARGET_PROPERTIES(vnchooks PROPERTIES COMPILE_FLAGS "-Wno-write-strings -Wno-unused-variable -Wno-unknown-pragmas")
SET_TARGET_PROPERTIES(vnchooks PROPERTIES LINK_FLAGS -Wl,-export-all-symbols)
TARGET_LINK_LIBRARIES(vnchooks -ladvapi32)

