// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include "NetworkObjectFilterProxyModel.h"

namespace Ui {
class LocationDialog;
}

class LocationDialog : public QDialog
{
	Q_OBJECT
public:
	explicit LocationDialog(QAbstractItemModel* locationListModel, QWidget* parent = nullptr);
	~LocationDialog() override;

	const QString& selectedLocation() const
	{
		return m_selectedLocation;
	}

private Q_SLOTS:
	void updateSearchFilter();
	void updateSelection(const QModelIndex& current, const QModelIndex& previous);

private:
	Ui::LocationDialog* ui;

	QSortFilterProxyModel m_sortFilterProxyModel{this};
	NetworkObjectFilterProxyModel m_networkObjectFilterProxyModel{this};
	QString m_selectedLocation;
};
