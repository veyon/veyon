// SPDX-FileCopyrightText: 2010-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

class VeyonConfiguration;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT ConfigurationManager : public QObject
{
	Q_OBJECT
public:
	explicit ConfigurationManager(QObject* parent = nullptr);

	bool clearConfiguration();
	bool applyConfiguration();
	// refuses to save configurations that validateConfiguration() rejects
	bool saveConfiguration();

	// empty if the configuration can be saved, else a user-visible error message
	static QString validateConfiguration(const VeyonConfiguration& configuration);

	const QString& errorString() const
	{
		return m_errorString;
	}

private:
	VeyonConfiguration& m_configuration;
	QString m_errorString;
};
