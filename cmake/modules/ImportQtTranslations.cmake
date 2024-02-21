# ImportQtTranslations.cmake - Copyright (c) 2020-2024 Tobias Junghans
#
# description: import translation files of Qt into build directory
# usage: import_qt_translations() with QT_TRANSLATIONS_DIR set


function(import_qt_translations)
	# find Qt's translation files
	set(QT_TRANSLATIONS_STAMP ${CMAKE_CURRENT_BINARY_DIR}/qttranslations.stamp)
	if(QT_TRANSLATIONS_DIR AND NOT EXISTS "${QT_TRANSLATIONS_STAMP}")
		message(STATUS "Processing Qt translation files in ${QT_TRANSLATIONS_DIR}")
		file(GLOB QT_TRANSLATIONS "${QT_TRANSLATIONS_DIR}/qt_*.qm")
		foreach(QT_TRANSLATION ${QT_TRANSLATIONS})
			if(NOT QT_TRANSLATION MATCHES "help")
				string(REPLACE "${QT_TRANSLATIONS_DIR}/" "" QT_TRANSLATION_FILE_NAME "${QT_TRANSLATION}")
				string(REPLACE "qt_" "qtbase_" QTBASE_TRANSLATION_FILE_NAME "${QT_TRANSLATION_FILE_NAME}")
				# is there qtbase-specific QM file?
				if(EXISTS "${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}")
					# then use it instead of (deprecated) QM file for all Qt modules
					file(COPY "${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
					message(STATUS "Imported Qt translation file: ${QT_TRANSLATIONS_DIR}/${QTBASE_TRANSLATION_FILE_NAME}")
				else()
					file(COPY ${QT_TRANSLATION} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
					message(STATUS "Imported Qt translation file: ${QT_TRANSLATION}")
				endif()
			endif()
		endforeach()
		file(WRITE "${QT_TRANSLATIONS_STAMP}" "1")
	endif()
endfunction()
