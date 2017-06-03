/*
 * ConfiguratorCore.h - global instances for the Veyon Configurator
 *
 * Copyright (c) 2010-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef CONFIGURATOR_CORE_H
#define CONFIGURATOR_CORE_H

#include "VeyonCore.h"

class VeyonConfiguration;

class ConfiguratorCore : public QObject
{
	Q_OBJECT
public:
	static bool applyConfiguration( const VeyonConfiguration &config );
	static int clearConfiguration();

	static bool createKeyPair( VeyonCore::UserRole role, const QString &destDir );
	static bool importPublicKey( VeyonCore::UserRole role, const QString &pubKey, const QString &destDir );

	static void informationMessage( const QString &title, const QString &msg );
	static void criticalMessage( const QString &title, const QString &msg );

	static bool silent;

private:
	static void configApplyError( const QString &msg );

};

#endif
