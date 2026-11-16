// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QFuture>

#include "VeyonCore.h"

class QWidget;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT ServiceControl : public QObject
{
	Q_OBJECT
public:
	ServiceControl(const QString& name,
				   const QString& filePath,
				   const QString& displayName,
				   QWidget* parent);

	bool isServiceRegistered();
	void registerService();
	void unregisterService();

	bool isServiceRunning();

	void startService();
	void stopService();
	void restartService();

private:
	using Operation = QFuture<void>;
	void serviceControl(const QString& title, const Operation& operation);
	void graphicalFeedback(const QString& title, const Operation& operation);
	void textFeedback(const QString& title, const Operation& operation);

	const QString m_name;
	const QString m_filePath;
	const QString m_displayName;

	QWidget* m_parent;
};
