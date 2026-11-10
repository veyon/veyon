/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <QProcessEnvironment>

struct LoginSession;

namespace GreeterEnvironment
{
// Only normalize display access for a logind greeter, never for a desktop.
QProcessEnvironment prepare(const LoginSession& session, QProcessEnvironment environment);
}
