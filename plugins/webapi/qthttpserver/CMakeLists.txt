include(BuildVeyonPlugin)

set(QTHTTPSERVER_SOURCES
	${qthttpserver_DIR}/src/3rdparty/http-parser/http_parser.c
	${qthttpserver_DIR}/src/httpserver/qhttpserverresponder.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouter_p.h
	${qthttpserver_DIR}/src/httpserver/qabstracthttpserver.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserver.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverresponse.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverfutureresponse.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverliterals_p.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouterrule.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouterrule.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouterviewtraits.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverresponse.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverfutureresponse.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouterrule_p.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverrequest.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverliterals.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverresponder.cpp
	${qthttpserver_DIR}/src/httpserver/qthttpserverglobal.h
	${qthttpserver_DIR}/src/httpserver/qhttpserver_p.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverrequest.h
	${qthttpserver_DIR}/src/httpserver/qhttpserver.h
	${qthttpserver_DIR}/src/httpserver/qabstracthttpserver.h
	${qthttpserver_DIR}/src/httpserver/qabstracthttpserver_p.h
	${qthttpserver_DIR}/src/httpserver/httpserver.pro
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouter.cpp
	${qthttpserver_DIR}/src/httpserver/qhttpserverrouter.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverresponse_p.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverrequest_p.h
	${qthttpserver_DIR}/src/httpserver/qhttpserverresponder_p.h
	${qthttpserver_DIR}/src/sslserver/qsslserver.cpp
	${qthttpserver_DIR}/src/sslserver/qsslserver.h
	${qthttpserver_DIR}/src/sslserver/qtsslserverglobal.h
	${qthttpserver_DIR}/src/sslserver/qsslserver_p.h
	)

add_library(qthttpserver SHARED ${QTHTTPSERVER_SOURCES})
set_source_files_properties(${QTHTTPSERVER_SOURCES} PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

target_compile_definitions(qthttpserver PRIVATE QT_USE_QSTRINGBUILDER QT_BUILD_HTTPSERVER_LIB QT_BUILD_SSLSERVER_LIB)

# qthttpserver does not build with QT_NO_CAST_FROM_ASCII
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_custom_target(qthttpserver_symlinks
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/httpserver
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/sslserver
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${qthttpserver_DIR}/src/httpserver ${CMAKE_CURRENT_BINARY_DIR}/httpserver/private
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${qthttpserver_DIR}/src/sslserver ${CMAKE_CURRENT_BINARY_DIR}/sslserver/private
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${qthttpserver_DIR}/src/httpserver ${CMAKE_CURRENT_BINARY_DIR}/QtHttpServer
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${qthttpserver_DIR}/src/sslserver ${CMAKE_CURRENT_BINARY_DIR}/QtSslServer
)

add_dependencies(qthttpserver qthttpserver_symlinks)

target_include_directories(qthttpserver PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}/httpserver
	${CMAKE_CURRENT_BINARY_DIR}/sslserver
	${qthttpserver_DIR}/src/httpserver
	${qthttpserver_DIR}/src/3rdparty/http-parser)

target_include_directories(qthttpserver PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(qthttpserver PRIVATE Qt5::CorePrivate Qt5::Concurrent Qt5::NetworkPrivate)

install(TARGETS qthttpserver DESTINATION ${VEYON_LIB_DIR})
