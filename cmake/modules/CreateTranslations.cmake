# CreateTranslations.cmake - Copyright (c) 2020-2021 Tobias Junghans
#
# description: create Qt translation files
# usage: create_translations(<TS FILES> <SOURCE FILES>)

function(create_translations name ts_files source_files)

	set(qm_targets "")
	foreach(ts_file ${ts_files})
		string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" ts_filename "${ts_file}")
		string(REPLACE ".ts" "" basename "${ts_filename}")
		set(ts_target "${basename}_ts")
		set(qm_target "${basename}_qm")
		set(qm_file "${CMAKE_CURRENT_BINARY_DIR}/${basename}.qm")
		add_custom_command(OUTPUT ${ts_file}
			COMMAND ${Qt5_LUPDATE_EXECUTABLE} -I${CMAKE_SOURCE_DIR}/core/include -locations none -no-obsolete ${source_files} -ts ${ts_file}
			DEPENDS ${source_files})
		add_custom_target(${ts_target} DEPENDS ${ts_file})
		# add command and target for generating/updating QM file if TS file is newer or no QM file exists yet
		add_custom_command(OUTPUT ${qm_file}
			COMMAND ${Qt5_LRELEASE_EXECUTABLE} ${ts_file} -qm ${qm_file}
			DEPENDS ${ts_file})
		add_custom_target(${qm_target} DEPENDS ${qm_file})

		list(APPEND qm_targets "${qm_target}")

		install(FILES ${qm_file} DESTINATION ${VEYON_INSTALL_DATA_DIR}/translations)
	endforeach()

	add_custom_target("${name}-translations" ALL DEPENDS "${qm_targets}")

endfunction()
