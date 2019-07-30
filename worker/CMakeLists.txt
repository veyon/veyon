INCLUDE(BuildApplication)
INCLUDE(WindowsBuildHelpers)

FILE(GLOB worker_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB worker_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

BUILD_APPLICATION(veyon-worker ${worker_SOURCES} ${worker_INCLUDES})

ADD_WINDOWS_RESOURCE(veyon-worker)
MAKE_GRAPHICAL_APP(veyon-worker)

COTIRE_VEYON(veyon-worker)
