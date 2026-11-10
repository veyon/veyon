/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QProcessEnvironment>

namespace KdeSessionControl
{
enum class Operation { Logout, Reboot, PowerOff };

bool isUnavailable(const QDBusMessage& reply);
QDBusMessage request(Operation operation, const QDBusConnection& sessionBus,
	const QProcessEnvironment& environment = QProcessEnvironment::systemEnvironment());
}
