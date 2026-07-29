include(BuildVeyonPlugin)

build_veyon_plugin(inventory
	NAME Inventory
	SOURCES
	InventoryPlugin.cpp InventoryPlugin.h
	InventoryMasterDialog.cpp InventoryMasterDialog.h InventoryMasterDialog.ui
)
