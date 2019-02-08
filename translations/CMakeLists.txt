# build translation files
FILE(GLOB veyon_LOCALES ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)
SET(ts_targets "")
SET(qm_targets "")
FILE(GLOB_RECURSE veyon_SOURCES ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.ui)
STRING(REGEX REPLACE "${CMAKE_SOURCE_DIR}/3rdparty[^;]+;?" "" veyon_SOURCES "${veyon_SOURCES}")

ADD_CUSTOM_TARGET(download-translations)

FOREACH(ts_file ${veyon_LOCALES})
    STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" ts_target "${ts_file}")
	STRING(REPLACE ".ts" ".qm" qm_target "${ts_target}")
	SET(qm_file "${CMAKE_CURRENT_BINARY_DIR}/${qm_target}")
	ADD_CUSTOM_TARGET(${ts_target} COMMAND ${Qt5_LUPDATE_EXECUTABLE} -I${CMAKE_SOURCE_DIR}/core/include -locations none -no-obsolete ${veyon_SOURCES} -ts ${ts_file})
	# add command and target for generating/updating QM file if TS file is newer or no QM file exists yet
	ADD_CUSTOM_COMMAND(OUTPUT ${qm_file} COMMAND ${Qt5_LRELEASE_EXECUTABLE} ${ts_file} -qm ${qm_file} DEPENDS ${ts_file})
	ADD_CUSTOM_TARGET(${qm_target} ALL DEPENDS ${qm_file})

	LIST(APPEND ts_targets "${ts_target}")
	LIST(APPEND qm_targets "${qm_target}")

	INSTALL(FILES ${qm_file} DESTINATION ${VEYON_INSTALL_DATADIR}/translations)

	# add target for downloading translation file from Transifex
	STRING(REPLACE ".ts" "" _translation "${ts_target}")
	IF(NOT ${_translation} STREQUAL "veyon")
	    SET(_download_translation_target "download-${_translation}")
		ADD_CUSTOM_TARGET(${_download_translation_target} COMMAND curl --user api:$ENV{TRANSIFEX_API_TOKEN} -o ${ts_file} -X GET "https://www.transifex.com/api/2/project/veyon/resource/veyon_41/translation/${_translation}/?mode=default\\&file")
		ADD_DEPENDENCIES(download-translations ${_download_translation_target})
	ENDIF()
ENDFOREACH()

ADD_CUSTOM_TARGET(update-locales)
FOREACH(_item ${ts_targets})
    ADD_DEPENDENCIES(update-locales ${_item})
ENDFOREACH(_item ${ts_targets})


# find Qt's translation files
SET(QT_TRANSLATIONS_STAMP ${CMAKE_CURRENT_BINARY_DIR}/qttranslations.stamp)
IF(NOT EXISTS "${QT_TRANSLATIONS_STAMP}")
	GET_TARGET_PROPERTY(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
	EXECUTE_PROCESS(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
						OUTPUT_STRIP_TRAILING_WHITESPACE
						OUTPUT_VARIABLE QT_TRANSLATIONS_DIR)
	FILE(WRITE "${QT_TRANSLATIONS_STAMP}" "1")
	IF(VEYON_BUILD_WIN32)
		FILE(GLOB QT_TRANSLATIONS "${QT_TRANSLATIONS_DIR}/qt_*.qm")
		FOREACH(QT_TRANSLATION ${QT_TRANSLATIONS})
			IF(NOT QT_TRANSLATION MATCHES "help")
				STRING(REPLACE "${QT_TRANSLATIONS_DIR}/" "" QT_TRANSLATION_FILE_NAME "${QT_TRANSLATION}")
				STRING(REPLACE "qt_" "qtbase_" QTBASE_TRANSLATION_FILE_NAME "${QT_TRANSLATION_FILE_NAME}")
				# is there qtbase-specific QM file?
				IF(EXISTS "${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}")
					# then use it instead of (deprecated) QM file for all Qt modules
					FILE(COPY "${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
					FILE(RENAME "${CMAKE_CURRENT_BINARY_DIR}/${QTBASE_TRANSLATION_FILE_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${QT_TRANSLATION_FILE_NAME}")
				ELSE()
					FILE(COPY ${QT_TRANSLATION} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
				ENDIF()
			ENDIF()
		ENDFOREACH()
	ENDIF()
ENDIF()
