// SPDX-FileCopyrightText: 2018-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>

template<typename Key>
class HashList : public QHash<Key, bool>
{
public:
	void append(const Key& key)
	{
		this->insert(key, true);
	}
};
