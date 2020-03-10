include(BuildVeyonApplication)
include(WindowsBuildHelpers)

file(GLOB worker_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB worker_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

build_veyon_application(veyon-worker ${worker_SOURCES} ${worker_INCLUDES})

add_windows_resource(veyon-worker)
make_graphical_app(veyon-worker)

cotire_veyon(veyon-worker)
