include(BuildVeyonApplication)

set(master_SOURCES
 src/main.cpp src/VeyonMaster.cpp src/VeyonMaster.h
 src/MainWindow.cpp src/MainWindow.h src/MasterComputerModel.cpp
 src/ComputerManager.cpp src/ComputerManager.h
 src/CheckableItemProxyModel.cpp src/CheckableItemProxyModel.h
 src/KExtraColumnsProxyModel.cpp src/KExtraColumnsProxyModel.h
 src/NetworkObjectFilterProxyModel.cpp src/NetworkObjectFilterProxyModel.h
 src/NetworkObjectOverlayDataModel.cpp src/NetworkObjectOverlayDataModel.h
 src/NetworkObjectTreeModel.cpp src/NetworkObjectTreeModel.h
 src/ComputerControlListModel.cpp src/ComputerControlListModel.h
 src/ComputerMonitoringModel.cpp src/ComputerMonitoringModel.h
 src/UserConfig.cpp src/UserConfig.h
 qml/master-qml.qrc)

set(master_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/master.qrc)
if(VEYON_DEBUG)
	set(master_RESOURCES ${master_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/resources/examples.qrc)
endif()

build_veyon_application(veyon-master
	NAME Master
	WINDOWS_ICON data/veyon-master.ico
	SOURCES
	${master_SOURCES}
	${master_RESOURCES})

if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
endif()
