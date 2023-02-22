include(CreateTranslations)
include(ImportQtTranslations)

file(GLOB veyon_translations ${CMAKE_CURRENT_SOURCE_DIR}/*.ts)
file(GLOB_RECURSE veyon_sources ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.ui ${CMAKE_SOURCE_DIR}/*.qml)
string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/3rdparty[^;]+;?" "" veyon_sources "${veyon_sources}")
string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/addons[^;]+;?" "" veyon_sources "${veyon_sources}")

create_translations(veyon "${veyon_translations}" "${veyon_sources}")
import_qt_translations()
