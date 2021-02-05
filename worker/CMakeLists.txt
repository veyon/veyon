include(BuildVeyonApplication)
include(WindowsBuildHelpers)

file(GLOB worker_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

build_veyon_application(veyon-worker ${worker_SOURCES})

add_windows_resource(veyon-worker)
make_graphical_app(veyon-worker)
