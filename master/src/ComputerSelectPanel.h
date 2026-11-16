// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QModelIndexList>
#include <QWidget>

class QSortFilterProxyModel;

namespace Ui {
class ComputerSelectPanel;
}

class ComputerManager;

class ComputerSelectPanel : public QWidget
{
	Q_OBJECT
public:
	explicit ComputerSelectPanel(ComputerManager& computerManager, QWidget* parent = nullptr);
	~ComputerSelectPanel() override;

	bool eventFilter(QObject* watched, QEvent* event) override;

private Q_SLOTS:
	void addLocation();
	void removeLocation();
	void saveList();
	void updateFilter();

private:
	void fetchAndExpandAll();
	void fetchAll(const QModelIndex& index);

	Ui::ComputerSelectPanel* ui;
	ComputerManager& m_computerManager;
	QSortFilterProxyModel* m_filterProxyModel;
	QString m_previousFilter;
	QModelIndexList m_expandedGroups;
};
