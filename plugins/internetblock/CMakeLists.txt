include(BuildVeyonPlugin)

set(INTERNETBLOCK_SOURCES
	InternetBlockFeaturePlugin.cpp
	InternetBlockFeaturePlugin.h
	InternetBlockConfiguration.h
	InternetBlockConfigurationPage.cpp
	InternetBlockConfigurationPage.h
	InternetBlockConfigurationPage.ui
	InternetBlockAddressRanges.cpp
	InternetBlockAddressRanges.h
	InternetBlocker.h
	internetblock.qrc
	)

if(VEYON_BUILD_WINDOWS)
	list(APPEND INTERNETBLOCK_SOURCES InternetBlockerWindows.cpp)
else()
	list(APPEND INTERNETBLOCK_SOURCES InternetBlockerLinux.cpp)
endif()

build_veyon_plugin(internetblock
	NAME InternetBlock
	SOURCES
	${INTERNETBLOCK_SOURCES}
	)

if(VEYON_BUILD_WINDOWS)
	# bundled netfw.h (not shipped by all MinGW versions)
	target_include_directories(internetblock PRIVATE ${CMAKE_SOURCE_DIR}/plugins/platform/windows)
	target_link_libraries(internetblock PRIVATE -lole32 -loleaut32)
	# INITGUID must only be seen by the firewall backend translation unit
	set_source_files_properties(InternetBlockerWindows.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)
endif()

test_veyon_plugin(internetblock InternetBlockTest)
if(WITH_TESTS)
	target_link_libraries(InternetBlockTest PRIVATE Qt${QT_MAJOR_VERSION}::Test)
	target_include_directories(InternetBlockTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
endif()
