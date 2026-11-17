include(BuildVeyonFuzzer)

build_veyon_fuzzer(pipewireframe
	main.cpp
	${CMAKE_SOURCE_DIR}/plugins/vncserver/pipewire/PipeWireFrame.cpp
	)

target_include_directories(pipewireframe PRIVATE ${CMAKE_SOURCE_DIR}/plugins/vncserver/pipewire ${Spa_INCLUDE_DIR})
