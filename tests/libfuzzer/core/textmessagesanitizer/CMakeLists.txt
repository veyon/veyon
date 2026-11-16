include(BuildVeyonFuzzer)

build_veyon_fuzzer(textmessagesanitizer
	main.cpp
	${CMAKE_SOURCE_DIR}/plugins/textmessage/TextMessageSanitizer.cpp
	../../common/init.cpp
	)

target_include_directories(textmessagesanitizer PRIVATE ${CMAKE_SOURCE_DIR}/plugins/textmessage)
