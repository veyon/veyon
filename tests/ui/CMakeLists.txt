if(WITH_TESTS)
 set(workspace_SOURCES
  VeyonMaster.cpp MainWindow.cpp MasterComputerModel.cpp
  ComputerManager.cpp ComputerControlListModel.cpp ComputerMonitoringModel.cpp
  CheckableItemProxyModel.cpp KExtraColumnsProxyModel.cpp NetworkObjectFilterProxyModel.cpp
  NetworkObjectOverlayDataModel.cpp NetworkObjectTreeModel.cpp UserConfig.cpp)
 list(TRANSFORM workspace_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/master/src/")
 add_executable(MasterWorkspaceTest MasterWorkspaceTest.cpp ${workspace_SOURCES}
  ${CMAKE_SOURCE_DIR}/master/src/MainWindow.h
  ${CMAKE_SOURCE_DIR}/master/resources/master.qrc
  ${CMAKE_SOURCE_DIR}/master/qml/master-qml.qrc)
 target_include_directories(MasterWorkspaceTest PRIVATE ${CMAKE_SOURCE_DIR}/master/src)
 target_compile_definitions(MasterWorkspaceTest PRIVATE VEYON_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
 target_link_libraries(MasterWorkspaceTest PRIVATE veyon-core Qt${QT_MAJOR_VERSION}::Test)
 set_default_target_properties(MasterWorkspaceTest)
 add_test(NAME MasterWorkspaceTest COMMAND MasterWorkspaceTest)
 set_tests_properties(MasterWorkspaceTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software" TIMEOUT 90)
endif()

if(WITH_TESTS)
 add_executable(QuickDialogTest QuickDialogTest.cpp)
 target_include_directories(QuickDialogTest PRIVATE ${CMAKE_SOURCE_DIR}/plugins/filetransfer)
 target_link_libraries(QuickDialogTest PRIVATE veyon-core filetransfer Qt${QT_MAJOR_VERSION}::Test)
 set_default_target_properties(QuickDialogTest)
 add_test(NAME QuickDialogTest COMMAND QuickDialogTest)
 set_tests_properties(QuickDialogTest PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
  TIMEOUT 90)
endif()
