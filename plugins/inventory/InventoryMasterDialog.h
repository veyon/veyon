#pragma once

#include <QDialog>
#include <QMap>
#include <QTreeWidgetItem>
#include "ComputerControlInterface.h"

namespace Ui { class InventoryMasterDialog; }

class InventoryMasterDialog : public QDialog
{
	Q_OBJECT
public:
	explicit InventoryMasterDialog(QWidget* parent = nullptr);
	~InventoryMasterDialog() override;

	void setComputers(const ComputerControlInterfaceList& computers);
	void addInventoryData(const QString& computerName, const QVariantMap& data);

private Q_SLOTS:
	void onRefreshClicked();
	void onExportClicked();

private:
	Ui::InventoryMasterDialog* ui;
	ComputerControlInterfaceList m_computers;
	QMap<QString, QTreeWidgetItem*> m_computerItems;
};
