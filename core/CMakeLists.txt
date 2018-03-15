FILE(GLOB veyoncore_INCLUDES
						${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
						${CMAKE_CURRENT_SOURCE_DIR}/include/Configuration/*.h
)
FILE(GLOB veyoncore_SOURCES
						${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
						${CMAKE_CURRENT_SOURCE_DIR}/src/Configuration/*.cpp
)

FILE(GLOB veyoncore_UI ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

# find Qt's translation files
SET(QT_TRANSLATIONS_QRC ${CMAKE_CURRENT_BINARY_DIR}/qttranslations.qrc)
IF(NOT EXISTS "${QT_TRANSLATIONS_QRC}")
	GET_TARGET_PROPERTY(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
	EXECUTE_PROCESS(COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
						OUTPUT_STRIP_TRAILING_WHITESPACE
						OUTPUT_VARIABLE QT_TRANSLATIONS_DIR)
	FILE(WRITE "${QT_TRANSLATIONS_QRC}" "<RCC><qresource prefix=\"/qttranslations\">\n")
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
				FILE(APPEND ${QT_TRANSLATIONS_QRC} "<file>${QT_TRANSLATION_FILE_NAME}</file>\n")
			ENDIF()
		ENDFOREACH()
	ELSE()
	        ADD_DEFINITIONS(-D'QT_TRANSLATIONS_DIR="${QT_TRANSLATIONS_DIR}"')
	ENDIF()
	FILE(APPEND "${QT_TRANSLATIONS_QRC}" "</qresource></RCC>")
ENDIF()


SET(libvncclient_SOURCES
	${libvncserver_DIR}/libvncclient/cursor.c
	${libvncserver_DIR}/libvncclient/listen.c
	${libvncserver_DIR}/libvncclient/rfbproto.c
	${libvncserver_DIR}/libvncclient/sockets.c
	${libvncserver_DIR}/libvncclient/tls_none.c
	${libvncserver_DIR}/libvncclient/vncviewer.c
	${libvncserver_DIR}/common/d3des.c
	${libvncserver_DIR}/common/turbojpeg.c)


QT5_WRAP_CPP(veyoncore_MOC_out ${veyoncore_INCLUDES})
QT5_WRAP_UI(veyoncore_UIC_out ${veyoncore_UI})
QT5_ADD_RESOURCES(veyoncore_RCC_out ${CMAKE_CURRENT_SOURCE_DIR}/core.qrc ${CMAKE_CURRENT_BINARY_DIR}/builddata.qrc ${CMAKE_CURRENT_BINARY_DIR}/qttranslations.qrc)

ADD_DEFINITIONS(-DBUILD_VEYON_CORE_LIBRARY)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include ${libvncserver_DIR}/common/)
ADD_LIBRARY(veyon-core SHARED ${veyoncore_SOURCES} ${veyoncore_INCLUDES} ${veyoncore_MOC_out} ${veyoncore_UIC_out} ${veyoncore_RCC_out} ${veyoncore_qt_qm_out} ${libvncclient_SOURCES})

SET_SOURCE_FILES_PROPERTIES(${libvncclient_SOURCES} PROPERTIES COMPILE_FLAGS "-Wno-unused-variable -Wno-unused-label -Wno-maybe-uninitialized -Wno-misleading-indentation -Wno-pointer-sign -Wno-int-in-bool-context")


IF(VEYON_BUILD_WIN32)
	# add Windows Socket library required by libvncclient
	TARGET_LINK_LIBRARIES(veyon-core -lws2_32)
	SET_TARGET_PROPERTIES(veyon-core PROPERTIES PREFIX "")
	INSTALL(TARGETS veyon-core RUNTIME DESTINATION ${VEYON_LIB_DIR})
ELSE()
	INSTALL(TARGETS veyon-core LIBRARY DESTINATION ${VEYON_LIB_DIR})
ENDIF(VEYON_BUILD_WIN32)

TARGET_LINK_LIBRARIES(veyon-core
	Qt5::Concurrent
	Qt5::Gui
	Qt5::Network
	Qt5::Widgets
	${ZLIB_LIBRARIES}
	${JPEG_LIBRARIES}
	${PNG_LIBRARIES}
	${LZO_LIBRARIES}
	${QCA_LIBRARY}
	${OPENSSL_LIBRARIES}
	)

ADD_DEPENDENCIES(veyon-core GenerateTranslationFiles)

COTIRE_VEYON(veyon-core)
