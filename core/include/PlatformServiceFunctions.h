/*
 * PlatformServiceFunctions.h - interface class for platform plugins
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

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformServiceFunctions
{
public:
	typedef enum StartModes {
		StartModeDisabled,
		StartModeManual,
		StartModeAuto,
		StartModeCount
	} StartMode;

	virtual QString veyonServiceName() const = 0;

	virtual bool isRegistered( const QString& name ) = 0;
	virtual bool isRunning( const QString& name ) = 0;
	virtual bool start( const QString& name ) = 0;
	virtual bool stop( const QString& name ) = 0;
	virtual bool install( const QString& name, const QString& filePath,
						  StartMode startMode, const QString& displayName ) = 0;
	virtual bool uninstall( const QString& name ) = 0;
	virtual bool setStartMode( const QString& name, StartMode startMode ) = 0;
	virtual bool runAsService( const QString& name, const std::function<void(void)>& serviceMain ) = 0;
	virtual void manageServerInstances() = 0;

};
