// SPDX-FileCopyrightText: 2009-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Configuration/Store.h"
#include "VeyonCore.h"

class QSettings;

namespace Configuration {

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT LocalStore : public Store
{
public:
	explicit LocalStore(Scope scope);

	void load(Object* obj) override;
	void flush(const Object* obj) override;
	bool isWritable() const override;
	void clear() override;

	QSettings* createSettingsObject() const;
};

}
