// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QJsonObject>

#include "Configuration/Store.h"
#include "VeyonCore.h"

namespace Configuration {

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT JsonStore : public Store
{
public:
	explicit JsonStore(Scope scope, const QString& file = {});

	void load(Object* obj) override;
	void flush(const Object* obj) override;
	bool isWritable() const override;
	void clear() override;

	// JSON tree as written to/read from configuration files (export format)
	static QJsonObject toJsonObject(const Object* obj);
	static void loadJsonObject(Object* obj, const QJsonObject& jsonObject);

private:
	QString configurationFilePath() const;

	QString m_file;
};

}
