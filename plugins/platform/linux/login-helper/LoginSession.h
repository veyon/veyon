/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QDBusConnection>
#include <QString>
#include <QVector>

struct LoginSession
{
	QString path;
	QString id;
	QString name;
	QString seat;
	QString service;
	QString type;
	QString display;
	QString state;
	QString sessionClass;
	uint uid = uint(-1);
	uint leader = 0;

	bool isGraphical() const;
	bool isOpen() const;
	bool isLightDmGreeter() const;
	bool isUser() const;
	static LoginSession read(const QDBusConnection& bus, const QString& path);
	static QVector<LoginSession> list(const QDBusConnection& bus, bool* ok);
	static QString forPid(const QDBusConnection& bus, uint pid);
};
