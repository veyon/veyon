// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Computer.h"

Computer::Computer(NetworkObject::Uid networkObjectUid,
				   const QString& displayName,
				   const QString& hostAddress,
				   const QString& macAddress,
				   const QString& location) :
	m_networkObjectUid(networkObjectUid),
	m_displayName(displayName),
	m_hostName(hostAddress),
	m_hostAddress(hostAddress),
	m_macAddress(macAddress),
	m_location(location)
{
}
