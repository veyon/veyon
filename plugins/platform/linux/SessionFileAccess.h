/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QString>

namespace SessionFileAccess
{
// LightDM may supply a root-owned cookie file to its greeter. The X11 Veyon
// server runs as root and may use it; ordinary user sessions retain the strict
// session-owner check. Never accept writable or non-regular authority files.
bool isXAuthoritySafe(const QString& path, uint sessionUid, bool isGreeter);
}
