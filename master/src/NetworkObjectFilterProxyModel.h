// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class NetworkObjectFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	explicit NetworkObjectFilterProxyModel(QObject* parent);

	void setGroupFilter(const QStringList& groupList);
	void setComputerExcludeFilter(const QStringList& computerExcludeList);
	void setEmptyGroupsExcluded(bool enabled);
	void setComputersExcluded(bool enabled);

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
	bool filterAcceptsRowRecursive(const QModelIndex& index) const;
	bool parentContainerAccepted(const QModelIndex& index) const;

	QStringList m_groupList;
	QStringList m_computerExcludeList;
	bool m_excludeEmptyGroups = false;
	bool m_excludeComputers = false;
};
