#include "NetworkDiscoveryDialog.h"
#include "ui_NetworkDiscoveryDialog.h"
#include <QListWidgetItem>

NetworkDiscoveryDialog::NetworkDiscoveryDialog(const QStringList& discoveredComputers, QWidget* parent) :
	QDialog(parent),
	ui(new Ui::NetworkDiscoveryDialog)
{
	ui->setupUi(this);

	for (const QString& computer : discoveredComputers) {
		auto item = new QListWidgetItem(computer, ui->listWidget);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Checked); // Default to checked
	}
}

NetworkDiscoveryDialog::~NetworkDiscoveryDialog()
{
	delete ui;
}

QStringList NetworkDiscoveryDialog::selectedComputers() const
{
	QStringList selected;
	for (int i = 0; i < ui->listWidget->count(); ++i) {
		auto item = ui->listWidget->item(i);
		if (item->checkState() == Qt::Checked) {
			selected.append(item->text());
		}
	}
	return selected;
}
