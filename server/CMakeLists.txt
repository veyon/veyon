INCLUDE(WindowsBuildHelpers)

FILE(GLOB server_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB server_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
QT5_WRAP_CPP(server_MOC_out ${server_INCLUDES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

ADD_EXECUTABLE(veyon-server ${server_SOURCES} ${server_INCLUDES} ${server_MOC_out})
TARGET_LINK_LIBRARIES(veyon-server veyon-core)

ADD_WINDOWS_RESOURCE(veyon-server)
MAKE_GRAPHICAL_APP(veyon-server)

INSTALL(TARGETS veyon-server RUNTIME DESTINATION bin)

TARGET_LINK_LIBRARIES(veyon-server
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
	)

COTIRE_VEYON(veyon-server)
