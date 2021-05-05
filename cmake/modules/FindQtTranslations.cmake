# FindQtTranslations.cmake - Copyright (c) 2020-2021 Tobias Junghans
#
# description: find translation files of Qt and prepare them for Windows build
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
						message(STATUS "Imported Qt translation file: ${QT_INSTALL_TRANSLATIONS}/${QTBASE_TRANSLATION_FILE_NAME}")
					else()
						file(COPY ${QT_TRANSLATION} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
						message(STATUS "Imported Qt translation file: ${QT_TRANSLATION}")
					endif()
				endif()
			endforeach()
			file(WRITE "${QT_TRANSLATIONS_STAMP}" "1")
		endif()
	endif()
endfunction()
