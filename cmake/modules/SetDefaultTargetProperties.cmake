macro(set_default_target_properties TARGET_NAME)
	set_property(TARGET ${TARGET_NAME} PROPERTY NO_SYSTEM_FROM_IMPORTED ON)
endmacro()
