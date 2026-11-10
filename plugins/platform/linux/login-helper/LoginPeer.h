/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QDBusConnection>
#include <QDBusConnectionInterface>

inline bool isLoginServicePeer(const QDBusConnection& bus, const QString& sender, const QString& service)
{
	if (sender.isEmpty() || !bus.interface())
	{
		return false;
	}
	const auto owner = bus.interface()->serviceOwner(service);
	const auto uid = bus.interface()->serviceUid(sender);
	return owner.isValid() && owner.value() == sender && uid.isValid() && uid.value() == 0;
}
