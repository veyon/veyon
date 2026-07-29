#pragma once

#include <QDialog>
#include <QTableWidget>
#include <QTimer>
#include "ComputerControlInterface.h"
#include "Feature.h"

class FocusTrackerDialog : public QDialog
{
	Q_OBJECT
public:
	explicit FocusTrackerDialog(QWidget* parent = nullptr);
	~FocusTrackerDialog() override;

	void setControlInterfaces(const ComputerControlInterfaceList& interfaces);
	void setFeatureUid(const Feature::Uid& uid);
	
	void updateComputerInfo(const QString& host, const QString& title);

private Q_SLOTS:
	void pollFocus();

private:
	QTableWidget* m_tableWidget;
	QTimer* m_timer;
	
	ComputerControlInterfaceList m_controlInterfaces;
	Feature::Uid m_featureUid;
	
	// Map from host/IP to row index
	QMap<QString, int> m_hostRowMap;
};
