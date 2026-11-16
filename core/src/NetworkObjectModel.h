// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractItemModel>

#include "NetworkObject.h"

class VEYON_CORE_EXPORT NetworkObjectModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit NetworkObjectModel(QObject* parent = nullptr) :
		QAbstractItemModel(parent)
	{
	}

	enum Role {
		CheckStateRole = Qt::CheckStateRole,
		NameRole = Qt::DisplayRole,
		UidRole = Qt::UserRole,
		TypeRole,
		HostAddressRole,
		MacAddressRole,
		DirectoryAddressRole,
		ParentUidRole
	};
};
