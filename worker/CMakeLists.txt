include(BuildVeyonApplication)
include(WindowsBuildHelpers)

file(GLOB worker_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB worker_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
build_veyon_application(veyon-worker ${worker_SOURCES} ${worker_INCLUDES})
target_link_libraries(veyon-worker veyon-core Qt5::Network)

add_windows_resource(veyon-worker)
make_graphical_app(veyon-worker)
