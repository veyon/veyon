/*
 * ImcCore.h - global instances for the iTALC Management Console
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _IMC_CORE_H
#define _IMC_CORE_H

#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QString>

#include "ItalcCore.h"

class ItalcConfiguration;
class MainWindow;

namespace ImcCore
{
	bool applyConfiguration( const ItalcConfiguration &config );
	void listConfiguration( const ItalcConfiguration &config );

	bool createKeyPair( ItalcCore::UserRole role, const QString &destDir );
	bool importPublicKey( ItalcCore::UserRole role,
							const QString &pubKey, const QString &destDir );

	QString icaFilePath();

	void informationMessage( const QString &title, const QString &msg );
	void criticalMessage( const QString &title, const QString &msg );

	// UI objects
	extern MainWindow * mainWindow;

} ;


#endif
