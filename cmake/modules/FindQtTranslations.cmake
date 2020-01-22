# FindQtTranslations.cmake - Copyright (c) 2020 Tobias Junghans
#
# description: find translation files of Qt and copy them on Windows to current binary dir
# usage: find_qt_translations()


function(find_qt_translations)
	# find Qt's translation files
	set(QT_TRANSLATIONS_STAMP ${CMAKE_CURRENT_BINARY_DIR}/qttranslations.stamp)
	if(NOT EXISTS "${QT_TRANSLATIONS_STAMP}")
		get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
		execute_process(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
							OUTPUT_STRIP_TRAILING_WHITESPACE
							OUTPUT_VARIABLE QT_INSTALL_TRANSLATIONS)
		message(STATUS "Found Qt translations: ${QT_INSTALL_TRANSLATIONS}")
		set(${QT_TRANSLATIONS_DIR} "${QT_INSTALL_TRANSLATIONS}" PARENT_SCOPE)
		file(WRITE "${QT_TRANSLATIONS_STAMP}" "1")
		if(WIN32)
			file(GLOB QT_TRANSLATIONS "${QT_INSTALL_TRANSLATIONS}/qt_*.qm")
			foreach(QT_TRANSLATION ${QT_TRANSLATIONS})
				if(NOT QT_TRANSLATION MATCHES "help")
					string(REPLACE "${QT_INSTALL_TRANSLATIONS}/" "" QT_TRANSLATION_FILE_NAME "${QT_TRANSLATION}")
					string(REPLACE "qt_" "qtbase_" QTBASE_TRANSLATION_FILE_NAME "${QT_TRANSLATION_FILE_NAME}")
					# is there qtbase-specific QM file?
					if(EXISTS "${QT_INSTALL_TRANSLATIONS}/${QTBASE_TRANSLATION_FILE_NAME}")
						# then use it instead of (deprecated) QM file for all Qt modules
						file(COPY "${QT_INSTALL_TRANSLATIONS}/${QTBASE_TRANSLATION_FILE_NAME}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
						file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/${QTBASE_TRANSLATION_FILE_NAME}" "${CMAKE_CURRENT_BINARY_DIR}/${QT_TRANSLATION_FILE_NAME}")
					else()
						file(COPY ${QT_TRANSLATION} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
					endif()
					message(STATUS "Imported Qt translation file: ${QT_TRANSLATION}")
				endif()
			endforeach()
		endif()
	endif()
endfunction()
