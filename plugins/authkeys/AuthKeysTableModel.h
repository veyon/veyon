// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractTableModel>

class AuthKeysManager;

class AuthKeysTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum Columns {
		ColumnKeyName,
		ColumnKeyType,
		ColumnKeyPairID,
		ColumnAccessGroup,
		ColumnCount
	};

	explicit AuthKeysTableModel(QObject* parent = nullptr);
	~AuthKeysTableModel() override;

	void reload();

	const QString& key(int row) const
	{
		return m_keys[row];
	}

	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	AuthKeysManager* m_manager;
	QStringList m_keys;
};
