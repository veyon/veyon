 MACRO(QT4_CREATE_TRANSLATION_FOR_QRC _qm_files _qrc_file)
      SET(_my_sources)
      SET(_my_tsfiles)
      FOREACH (_file ${ARGN})
         GET_FILENAME_COMPONENT(_ext ${_file} EXT)
         GET_FILENAME_COMPONENT(_abs_FILE ${_file} ABSOLUTE)
         IF(_ext MATCHES "ts")
           LIST(APPEND _my_tsfiles ${_abs_FILE})
         ELSE(_ext MATCHES "ts")
           LIST(APPEND _my_sources ${_abs_FILE})
         ENDIF(_ext MATCHES "ts")
      ENDFOREACH(_file)
      FOREACH(_ts_file ${_my_tsfiles})
        ADD_CUSTOM_COMMAND(OUTPUT ${_ts_file}
           COMMAND ${QT_LUPDATE_EXECUTABLE}
           ARGS ${_my_sources} -ts ${_ts_file} DEPENDS ${_qrc_file})
      ENDFOREACH(_ts_file)
      QT4_ADD_TRANSLATION_FOR_QRC(${_qm_files} ${_my_tsfiles})
ENDMACRO(QT4_CREATE_TRANSLATION_FOR_QRC)

MACRO(QT4_ADD_TRANSLATION_FOR_QRC _qm_files)
      FOREACH (_current_FILE ${ARGN})
         GET_FILENAME_COMPONENT(_abs_FILE ${_current_FILE} ABSOLUTE)
         GET_FILENAME_COMPONENT(qm_path ${_abs_FILE} PATH)
         GET_FILENAME_COMPONENT(qm ${_abs_FILE} NAME_WE)
         SET(qm "${qm_path}/${qm}.qm")

         ADD_CUSTOM_COMMAND(OUTPUT ${qm}
            COMMAND ${QT_LRELEASE_EXECUTABLE}
            ARGS ${_abs_FILE} -qm ${qm}
            DEPENDS ${_abs_FILE}
         )
         SET(${_qm_files} ${${_qm_files}} ${qm})
      ENDFOREACH (_current_FILE)
ENDMACRO(QT4_ADD_TRANSLATION_FOR_QRC)

MACRO(QT4_TRANSLATIONS_FOR_QRC _qrc_file)
	SET(TS_FILES)
	FOREACH(_lang ${SUPPORTED_LANGUAGES})
		LIST(APPEND TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/${_lang}.ts")
	ENDFOREACH(_lang ${SUPPORTED_LANGUAGES})
	QT4_CREATE_TRANSLATION_FOR_QRC(_qm_files ${_qrc_file} ${ARGN} ${TS_FILES})
ENDMACRO(QT4_TRANSLATIONS_FOR_QRC _qrc_file)

