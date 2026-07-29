#include "InventoryMasterDialog.h"
#include "ui_InventoryMasterDialog.h"
#include "InventoryPlugin.h"
#include "FeatureMessage.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

InventoryMasterDialog::InventoryMasterDialog(QWidget* parent) :
	QDialog(parent),
	ui(new Ui::InventoryMasterDialog)
{
	ui->setupUi(this);
	ui->inventoryTree->header()->setStretchLastSection(true);

	connect(ui->refreshButton, &QPushButton::clicked, this, &InventoryMasterDialog::onRefreshClicked);
	connect(ui->exportButton, &QPushButton::clicked, this, &InventoryMasterDialog::onExportClicked);
}

InventoryMasterDialog::~InventoryMasterDialog()
{
	delete ui;
}

void InventoryMasterDialog::setComputers(const ComputerControlInterfaceList& computers)
{
	m_computers = computers;
}

void InventoryMasterDialog::onRefreshClicked()
{
	ui->inventoryTree->clear();
	m_computerItems.clear();

	for (auto& computer : m_computers)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(ui->inventoryTree);
		item->setText(0, computer->computerName());
		item->setText(1, tr("Waiting for data..."));
		m_computerItems.insert(computer->computerName(), item);

		FeatureMessage msg(Feature::Uid(INVENTORY_FEATURE_UID), InventoryCommand::GetInventory);
		computer->sendFeatureMessage(msg);
	}
}

void InventoryMasterDialog::addInventoryData(const QString& computerName, const QVariantMap& data)
{
	if (!m_computerItems.contains(computerName)) return;

	QTreeWidgetItem* parentItem = m_computerItems.value(computerName);
	parentItem->setText(1, tr("Data received"));

	qDeleteAll(parentItem->takeChildren());

	for (auto it = data.constBegin(); it != data.constEnd(); ++it)
	{
		QTreeWidgetItem* child = new QTreeWidgetItem(parentItem);
		child->setText(0, it.key());
		child->setText(1, it.value().toString());
	}
	parentItem->setExpanded(true);
}

void InventoryMasterDialog::onExportClicked()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Export to CSV"), QStringLiteral("veyon_inventory.csv"), tr("CSV Files (*.csv);;All Files (*)"));
	if (fileName.isEmpty()) return;

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		QMessageBox::critical(this, tr("Error"), tr("Could not open file for writing!"));
		return;
	}

	QTextStream out(&file);
	out << QStringLiteral("Computer,Property,Value\n");

	for (int i = 0; i < ui->inventoryTree->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* computerItem = ui->inventoryTree->topLevelItem(i);
		QString compName = computerItem->text(0);
		for (int j = 0; j < computerItem->childCount(); ++j)
		{
			QTreeWidgetItem* child = computerItem->child(j);
			out << QStringLiteral("\"") << compName << QStringLiteral("\",\"") << child->text(0) << QStringLiteral("\",\"") << child->text(1) << QStringLiteral("\"\n");
		}
	}

	file.close();
	QMessageBox::information(this, tr("Success"), tr("Inventory successfully exported to CSV!"));
}
