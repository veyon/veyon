find_package(PAM REQUIRED)

set(CMAKE_SKIP_BUILD_RPATH  TRUE)
set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

add_executable(veyon-auth-helper ${CMAKE_CURRENT_SOURCE_DIR}/VeyonAuthHelper.cpp)

set_default_target_properties(veyon-auth-helper)

target_include_directories(veyon-auth-helper PRIVATE ${PAM_INCLUDE_DIR})
target_link_libraries(veyon-auth-helper PRIVATE ${PAM_LIBRARY} Qt${QT_MAJOR_VERSION}::Core)

install(TARGETS veyon-auth-helper RUNTIME DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE SETUID GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
