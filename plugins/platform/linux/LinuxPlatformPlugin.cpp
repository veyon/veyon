/*
 * LinuxPlatformPlugin.cpp - implementation of LinuxPlatformPlugin class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <csignal>
#include <execinfo.h>

#include "LinuxPlatformPlugin.h"
#include "LinuxPlatformConfiguration.h"
#include "LinuxPlatformConfigurationPage.h"

LinuxPlatformPlugin::LinuxPlatformPlugin( QObject* parent ) :
	QObject( parent )
{
	// make sure to load global config from default config dirs independent of environment variables
	qunsetenv( "XDG_CONFIG_DIRS" );

	// don't abort with SIGPIPE when writing to closed sockets e.g. while shutting down VncConnection
	::signal(SIGPIPE, SIG_IGN);

	::signal(SIGKILL, abort);
	::signal(SIGBUS, abort);
	::signal(SIGSEGV, abort);
}



LinuxPlatformPlugin::~LinuxPlatformPlugin()
{
	m_linuxInputDeviceFunctions.enableInputDevices();
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
		list.append(QStringLiteral("\t %1").arg( QLatin1String(humanReadableFrames[i])));
	}

	free(humanReadableFrames);

	return list.join(QLatin1String("\n"));
}


IMPLEMENT_CONFIG_PROXY(LinuxPlatformConfiguration)
