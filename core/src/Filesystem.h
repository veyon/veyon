// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "VeyonCore.h"

// clazy:excludeall=rule-of-three

// clazy:excludeall=ctor-missing-parent-argument
class VEYON_CORE_EXPORT Filesystem : public QObject
{
	Q_OBJECT
public:
	static QString safeJoin(const QString& baseDir, const QString& relativePath);
	static bool isSafePathComponent(const QString& name);

	QString expandPath(QString path) const;
	QString shrinkPath(QString path) const;
	bool ensurePathExists(const QString& path) const;

	QString privateKeyPath(const QString& name) const;
	QString publicKeyPath(const QString& name) const;

	QString screenshotDirectoryPath() const;

	QString serverFilePath() const;
	QString workerFilePath() const;

Q_SIGNALS:
	void screenshotDirectoryModified();
};
