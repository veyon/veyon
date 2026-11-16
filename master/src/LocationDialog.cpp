// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LocationDialog.h"

#include "ui_LocationDialog.h"

LocationDialog::LocationDialog(QAbstractItemModel* locationListModel, QWidget* parent) :
	QDialog(parent),
	ui(new Ui::LocationDialog)
{
	ui->setupUi(this);

	m_networkObjectFilterProxyModel.setSourceModel(locationListModel);
	m_networkObjectFilterProxyModel.setComputersExcluded(true);

	m_sortFilterProxyModel.setSourceModel(&m_networkObjectFilterProxyModel);
	m_sortFilterProxyModel.setRecursiveFilteringEnabled(true);
	m_sortFilterProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_sortFilterProxyModel.sort(0);

	ui->treeView->setModel(&m_sortFilterProxyModel);

	connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &LocationDialog::updateSelection);
	connect(ui->treeView, &QTreeView::activated,
			this, &LocationDialog::accept);

	updateSearchFilter();
}



LocationDialog::~LocationDialog()
{
	delete ui;
}



void LocationDialog::updateSearchFilter()
{
	ui->treeView->expandAll();

	m_sortFilterProxyModel.setFilterRegularExpression(ui->filterLineEdit->text());

	ui->treeView->selectionModel()->setCurrentIndex(m_sortFilterProxyModel.index(0, 0),
													QItemSelectionModel::ClearAndSelect);
}



void LocationDialog::updateSelection(const QModelIndex& current, const QModelIndex& previous)
{
	Q_UNUSED(previous)

	m_selectedLocation = m_sortFilterProxyModel.data(current).toString();
}
