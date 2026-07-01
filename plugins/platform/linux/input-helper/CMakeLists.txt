add_executable(veyon-input-helper input-block-helper.c)

set_default_target_properties(veyon-input-helper)

set_target_properties(veyon-input-helper PROPERTIES
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)

install(TARGETS veyon-input-helper RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE SETUID
	GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
