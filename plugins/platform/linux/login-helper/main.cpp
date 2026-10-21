/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "LightDmGreeter.h"
#include "LoginService.h"

#include <QCoreApplication>
#include <QDebug>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <csignal>
#include <unistd.h>

int main(int argc, char** argv)
{
	const rlimit noCore {0, 0};
	if (prctl(PR_SET_DUMPABLE, 0) != 0 || setrlimit(RLIMIT_CORE, &noCore) != 0)
	{
		return 1;
	}
	std::signal(SIGPIPE, SIG_IGN);
	QCoreApplication app(argc, argv);
	const auto arguments = app.arguments();
	if (arguments.size() >= 4 && arguments.at(1) == QLatin1String("--lightdm-greeter") && arguments.at(2) == QLatin1String("--"))
	{
		LightDmGreeter greeter;
		return greeter.start(arguments.mid(3)) ? app.exec() : 1;
	}
	if (arguments.size() != 2 || arguments.at(1) != QLatin1String("--service"))
	{
		qCritical() << "Usage: veyon-login-helper --service | --lightdm-greeter -- <original greeter> [arguments]";
		return 1;
	}
	if (getuid() != 0)
	{
		qCritical() << "The system login helper must run as root";
		return 1;
	}
	LoginService service;
	if (!service.start())
	{
		qCritical() << "Could not register io.veyon.LoginHelper on the system bus";
		return 1;
	}
	return app.exec();
}
