/*
 * PlatformFilesystemFunctions.h - interface class for platform filesystem
 *
 * Copyright (c) 2018-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef PLATFORM_FILESYSTEM_FUNCTIONS_H
#define PLATFORM_FILESYSTEM_FUNCTIONS_H

#include <QFile>

#include "PlatformPluginInterface.h"

// clazy:excludeall=copyable-polymorphic

class PlatformFilesystemFunctions
{
public:
	virtual QString personalAppDataPath() const = 0;
	virtual QString globalAppDataPath() const = 0;
	virtual QString globalTempPath() const = 0;

	virtual QString fileOwnerGroup( const QString& filePath ) = 0;
	virtual bool setFileOwnerGroup( const QString& filePath, const QString& ownerGroup ) = 0;
	virtual bool setFileOwnerGroupPermissions( const QString& filePath, QFile::Permissions permissions ) = 0;


};

#endif // PLATFORM_FILESYSTEM_FUNCTIONS_H
