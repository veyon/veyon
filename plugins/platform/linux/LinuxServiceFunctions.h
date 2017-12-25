/*
 * LinuxServiceFunctions.h - declaration of LinuxServiceFunctions class
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

#ifndef LINUX_SERVICE_FUNCTIONS_H
#define LINUX_SERVICE_FUNCTIONS_H

#include "PlatformServiceFunctions.h"

// clazy:excludeall=copyable-polymorphic

class LinuxServiceFunctions : public PlatformServiceFunctions
{
public:
	bool isRegistered( const QString& name ) override;
	bool isRunning( const QString& name ) override;
	bool start( const QString& name ) override;
	bool stop( const QString& name ) override;
	bool install( const QString& name, const QString& serviceFilePath,
				  StartMode startMode, const QString& displayName) override;
	bool uninstall( const QString& name ) override;
	bool setStartMode( const QString& name, StartMode startMode ) override;
	bool runAsService( const QString& name, std::function<void(void)> serviceMain ) override;
	void manageServerInstances() override;

};

#endif // LINUX_SERVICE_FUNCTIONS_H
