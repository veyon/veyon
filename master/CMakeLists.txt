include(BuildVeyonApplication)

set(master_SOURCES
	src/CheckableItemProxyModel.cpp
	src/CheckableItemProxyModel.h
	src/ComputerControlListModel.cpp
	src/ComputerControlListModel.h
	src/ComputerItemDelegate.cpp
	src/ComputerItemDelegate.h
	src/ComputerManager.cpp
	src/ComputerManager.h
	src/ComputerMonitoringModel.cpp
	src/ComputerMonitoringModel.h
	src/ComputerMonitoringView.cpp
	src/ComputerMonitoringView.h
	src/ComputerMonitoringWidget.cpp
	src/ComputerMonitoringWidget.h
	src/ComputerSelectPanel.cpp
	src/ComputerSelectPanel.h
	src/ComputerSelectPanel.ui
	src/ComputerZoomWidget.cpp
	src/ComputerZoomWidget.h
	src/DocumentationFigureCreator.cpp
	src/DocumentationFigureCreator.h
	src/FlexibleListView.cpp
	src/FlexibleListView.h
	src/KExtraColumnsProxyModel.cpp
	src/KExtraColumnsProxyModel.h
	src/LocationDialog.cpp
	src/LocationDialog.h
	src/LocationDialog.ui
	src/main.cpp
	src/MainToolBar.cpp
	src/MainToolBar.h
	src/MainWindow.cpp
	src/MainWindow.h
	src/MainWindow.ui
	src/NetworkObjectFilterProxyModel.cpp
	src/NetworkObjectFilterProxyModel.h
	src/NetworkObjectOverlayDataModel.cpp
	src/NetworkObjectOverlayDataModel.h
	src/NetworkObjectTreeModel.cpp
	src/NetworkObjectTreeModel.h
	src/ScreenshotManagementPanel.cpp
	src/ScreenshotManagementPanel.h
	src/ScreenshotManagementPanel.ui
	src/SlideshowModel.cpp
	src/SlideshowModel.h
	src/SlideshowPanel.cpp
	src/SlideshowPanel.h
	src/SlideshowPanel.ui
	src/SpotlightModel.cpp
	src/SpotlightModel.h
	src/SpotlightPanel.cpp
	src/SpotlightPanel.h
	src/SpotlightPanel.ui
	src/UserConfig.cpp
	src/UserConfig.h
	src/VeyonMaster.cpp
	src/VeyonMaster.h
	)

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
