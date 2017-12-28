/*
 * LinuxCoreFunctions.h - declaration of LinuxCoreFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef LINUX_CORE_FUNCTIONS_H
#define LINUX_CORE_FUNCTIONS_H

#include "PlatformCoreFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxCoreFunctions : public PlatformCoreFunctions
{
public:
	QString personalAppDataPath() const override;
	QString globalAppDataPath() const override;
	void initNativeLoggingSystem( const QString& appName ) override;
	void writeToNativeLoggingSystem( const QString& message, Logger::LogLevel loglevel ) override;

	void reboot() override;
	void powerDown() override;

	void raiseWindow( QWidget* widget ) override;

	QString activeDesktopName() override;

	bool runProgramAsUser( const QString& program, const QString& username,
						   const QString& desktop = QString() ) override;


};

#endif // LINUX_CORE_FUNCTIONS_H
