set(vnchooks_SOURCES
	${ultravnc_DIR}/winvnc/vnchooks/VNCHooks.cpp
	${ultravnc_DIR}/winvnc/vnchooks/SharedData.cpp
	${ultravnc_DIR}/winvnc/vnchooks/vnchooks.rc
)

add_library(vnchooks MODULE ${vnchooks_SOURCES})
set_source_files_properties(${vnchooks_SOURCES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)
set_default_target_properties(vnchooks)
set_target_properties(vnchooks PROPERTIES PREFIX "")
set_target_properties(vnchooks PROPERTIES COMPILE_FLAGS "-Wno-write-strings -Wno-unused-variable -Wno-unknown-pragmas")
set_target_properties(vnchooks PROPERTIES LINK_FLAGS -Wl,-export-all-symbols)
target_link_libraries(vnchooks PRIVATE -ladvapi32)
