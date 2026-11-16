// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIdentityProxyModel>
#include <QJsonArray>
#include <QUuid>

class CheckableItemProxyModel : public QIdentityProxyModel
{
	Q_OBJECT
public:
	explicit CheckableItemProxyModel(int uidRole, QObject* parent = nullptr);

	void setException(int exceptionRole, const QVariant& exceptionFilterData);

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

	void updateNewRows(const QModelIndex& parent, int first, int last);

	QJsonArray saveStates();
	void loadStates(const QJsonArray& data);

private:
	QUuid indexToUuid(const QModelIndex& index) const;
	bool setChildData(const QModelIndex& index, Qt::CheckState checkState);
	void setParentData(const QModelIndex& index, Qt::CheckState checkState);

	int m_uidRole;
	int m_exceptionRole;
	QVariant m_exceptionData;
	QHash<QUuid, Qt::CheckState> m_checkStates;
};
