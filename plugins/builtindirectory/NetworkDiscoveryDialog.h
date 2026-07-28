#pragma once

#include <QDialog>
#include <QStringList>

namespace Ui {
class NetworkDiscoveryDialog;
}

class NetworkDiscoveryDialog : public QDialog
{
	Q_OBJECT
public:
	explicit NetworkDiscoveryDialog(const QStringList& discoveredComputers, QWidget* parent = nullptr);
	~NetworkDiscoveryDialog() override;

	QStringList selectedComputers() const;

private:
	Ui::NetworkDiscoveryDialog *ui;
};
