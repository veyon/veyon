# fails if the server ⇄ worker channel still uses loopback TCP or a token outside the IPC handshake

set(files
	${VEYON_SOURCE_DIR}/core/src/FeatureWorkerManager.h
	${VEYON_SOURCE_DIR}/core/src/FeatureWorkerManager.cpp
	${VEYON_SOURCE_DIR}/worker/src/FeatureWorkerManagerConnection.h
	${VEYON_SOURCE_DIR}/worker/src/FeatureWorkerManagerConnection.cpp
	${VEYON_SOURCE_DIR}/worker/src/VeyonWorker.cpp
	${VEYON_SOURCE_DIR}/worker/src/main.cpp)

foreach(file ${files})
	file(READ ${file} content)
	foreach(pattern "QTcpServer" "QTcpSocket" "featureWorkerManagerPort" "AuthToken" "StandardInputReader")
		string(FIND "${content}" "${pattern}" position)
		if(NOT position EQUAL -1)
			message(FATAL_ERROR "${file} still uses ${pattern}")
		endif()
	endforeach()
endforeach()

message(STATUS "worker channel uses local IPC only")
