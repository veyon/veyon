INCLUDE(WindowsBuildHelpers)

FILE(GLOB worker_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
FILE(GLOB worker_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

QT5_WRAP_CPP(worker_MOC_out ${worker_INCLUDES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
ADD_EXECUTABLE(veyon-worker ${worker_UIC_out} ${worker_SOURCES} ${worker_INCLUDES} ${worker_MOC_out})
TARGET_LINK_LIBRARIES(veyon-worker veyon-core Qt5::Network)

ADD_WINDOWS_RESOURCE(veyon-worker)
MAKE_GRAPHICAL_APP(veyon-worker)

INSTALL(TARGETS veyon-worker RUNTIME DESTINATION bin)

COTIRE_VEYON(veyon-worker)
