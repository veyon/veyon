// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AccessControlGracePeriod.h"


AccessControlGracePeriod::AccessControlGracePeriod(const Clock& clock) :
	m_clock(clock)
{
}



bool AccessControlGracePeriod::keepAccess(ClientId client, const QString& description)
{
	const auto now = m_clock();

	const auto it = m_graceStart.constFind(client);
	if (it == m_graceStart.constEnd())
	{
		vWarning() << "directory for access control not available - keeping access of" << description
				   << "for up to" << GracePeriod / 1000 << "s";
		m_graceStart.insert(client, now);
		return true;
	}

	if (now - it.value() >= GracePeriod)
	{
		vWarning() << "directory for access control still not available - closing connection of" << description;
		m_graceStart.remove(client);
		return false;
	}

	return true;
}



void AccessControlGracePeriod::remove(ClientId client, const QString& description)
{
	if (m_graceStart.remove(client) > 0 && description.isEmpty() == false)
	{
		vInfo() << "access control for" << description << "possible again";
	}
}
