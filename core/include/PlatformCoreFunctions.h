/*
 * PlatformCoreFunctions.h - interface class for platform plugins
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QFile>

#include "Logger.h"
#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformCoreFunctions
{
public:
	virtual ~PlatformCoreFunctions() = default;

	virtual bool applyConfiguration() = 0;

	virtual void initNativeLoggingSystem( const QString& appName ) = 0;
	virtual void writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel ) = 0;

	virtual void reboot() = 0;
	virtual void powerDown( bool installUpdates ) = 0;

	virtual void raiseWindow( QWidget* widget ) = 0;

	virtual void disableScreenSaver() = 0;
	virtual void restoreScreenSaverSettings() = 0;

	virtual void setSystemUiState( bool enabled ) = 0;

	virtual QString activeDesktopName() = 0;

	virtual bool isRunningAsAdmin() const = 0;
	virtual bool runProgramAsAdmin( const QString& program, const QStringList& parameters ) = 0;

	virtual bool runProgramAsUser( const QString& program,
								   const QStringList& parameters,
								   const QString& username,
								   const QString& desktop ) = 0;

	virtual QString genericUrlHandler() const = 0;

};
