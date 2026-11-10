/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QStringList>

namespace LightDmSession
{
bool isAvailable(const QString& session, uint greeterLeader);
bool isAvailable(const QString& session, const QStringList& configurationFiles, const QStringList& defaultDirectories);
}
