# build translation files
file(GLOB veyon_LOCALES ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)
set(ts_targets "")
set(qm_targets "")
file(GLOB_RECURSE veyon_SOURCES ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.ui)
string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/3rdparty[^;]+;?" "" veyon_SOURCES "${veyon_SOURCES}")

foreach(ts_file ${veyon_LOCALES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" ts_target "${ts_file}")
	string(REPLACE ".ts" ".qm" qm_target "${ts_target}")
	set(qm_file "${CMAKE_CURRENT_BINARY_DIR}/${qm_target}")
	add_custom_target(${ts_target} COMMAND ${Qt5_LUPDATE_EXECUTABLE} -I${CMAKE_SOURCE_DIR}/core/include -locations none -no-obsolete ${veyon_SOURCES} -ts ${ts_file})
	# add command and target for generating/updating QM file if TS file is newer or no QM file exists yet
	add_custom_command(OUTPUT ${qm_file} COMMAND ${Qt5_LRELEASE_EXECUTABLE} ${ts_file} -qm ${qm_file} DEPENDS ${ts_file})
	add_custom_target(${qm_target} ALL DEPENDS ${qm_file})

	list(APPEND ts_targets "${ts_target}")
	list(APPEND qm_targets "${qm_target}")

	install(FILES ${qm_file} DESTINATION ${VEYON_INSTALL_DATA_DIR}/translations)
endforeach()

add_custom_target(update-locales)
foreach(_item ${ts_targets})
    ADD_DEPENDENCIES(update-locales ${_item})
endforeach()


# find Qt's translation files
set(QT_TRANSLATIONS_STAMP ${CMAKE_CURRENT_BINARY_DIR}/qttranslations.stamp)
if(NOT EXISTS "${QT_TRANSLATIONS_STAMP}")
	get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
	execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
						OUTPUT_STRIP_TRAILING_WHITESPACE
						OUTPUT_VARIABLE QT_TRANSLATIONS_DIR)
	file(WRITE "${QT_TRANSLATIONS_STAMP}" "1")
	if(VEYON_BUILD_WIN32)
		file(GLOB QT_TRANSLATIONS "${QT_TRANSLATIONS_DIR}/qt_*.qm")
		foreach(QT_TRANSLATION ${QT_TRANSLATIONS})
			if(NOT QT_TRANSLATION MATCHES "help")
				string(REPLACE "${QT_TRANSLATIONS_DIR}/" "" QT_TRANSLATION_FILE_NAME "${QT_TRANSLATION}")
				string(REPLACE "qt_" "qtbase_" QTBASE_TRANSLATION_FILE_NAME "${QT_TRANSLATION_FILE_NAME}")
				# is there qtbase-specific QM file?
				if(EXISTS "${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}")
					# then use it instead of (deprecated) QM file for all Qt modules
					file(COPY "${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
					file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/${QTBASE_TRANSLATION_FILE_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${QT_TRANSLATION_FILE_NAME}")
				else()
					file(COPY ${QT_TRANSLATION} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
				endif()
			endif()
		endforeach()
	endif()
endif()
