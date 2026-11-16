// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LinuxPlatformConfiguration.h"
#include "LinuxPlatformConfigurationPage.h"
#include "LinuxPlatformPlugin.h"

#include <csignal>
#include <execinfo.h>

LinuxPlatformPlugin::LinuxPlatformPlugin(QObject* parent) :
	QObject(parent)
{
	// make sure to load global config from default config dirs independent of environment variables
	qunsetenv("XDG_CONFIG_DIRS");

	// don't abort with SIGPIPE when writing to closed sockets e.g. while shutting down VncConnection
	::signal(SIGPIPE, SIG_IGN);

	::signal(SIGKILL, abort);
	::signal(SIGBUS, abort);
	::signal(SIGSEGV, abort);
}



LinuxPlatformPlugin::~LinuxPlatformPlugin()
{
}



ConfigurationPage* LinuxPlatformPlugin::createConfigurationPage()
{
	return new LinuxPlatformConfigurationPage();
}



void LinuxPlatformPlugin::abort(int signal)
{
	vCritical() << "Received signal" << signal;

	qCritical().noquote() << formattedBacktraceString();

	qFatal("Aborting due to severe error");
	::abort();
}



QString LinuxPlatformPlugin::formattedBacktraceString()
{
	static constexpr int BackTraceMaxDepth = 20;

	void* stackFrame[BackTraceMaxDepth + 1];
	const auto frameCount = backtrace(stackFrame, BackTraceMaxDepth + 1);

	char** humanReadableFrames = backtrace_symbols(stackFrame, frameCount);

	QStringList list{QLatin1String("BACKTRACE:")};
	list.reserve(frameCount);
	for (int i = 1; i < frameCount; i++)
	{
		list.append(QStringLiteral("\t %1").arg(QLatin1String(humanReadableFrames[i])));
	}

	free(humanReadableFrames);

	return list.join(QLatin1String("\n"));
}


IMPLEMENT_CONFIG_PROXY(LinuxPlatformConfiguration)
