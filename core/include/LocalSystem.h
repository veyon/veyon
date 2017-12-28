/*
 * LocalSystem.h - misc. platform-specific stuff
 *
 * Copyright (c) 2006-2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef LOCAL_SYSTEM_H
#define LOCAL_SYSTEM_H

#include "VeyonCore.h"

#ifdef VEYON_BUILD_WIN32
#include <windef.h>
#endif

#define QDTNS(x)	QDir::toNativeSeparators(x)

class QWidget;

// clazy:excludeall=rule-of-three

namespace LocalSystem
{
	class VEYON_CORE_EXPORT Process
	{
	public:
		static bool isRunningAsAdmin();
		static bool runAsAdmin( const QString &proc, const QString &parameters );

	} ;


	class VEYON_CORE_EXPORT Path
	{
	public:
		static QString expand( QString path );
		static QString shrink( QString path );
		static bool ensurePathExists( const QString &path );

		static QString privateKeyPath( VeyonCore::UserRoles role,
												QString baseDir = QString() );
		static QString publicKeyPath( VeyonCore::UserRoles role,
												QString baseDir = QString() );
	} ;


#ifdef VEYON_BUILD_WIN32
	BOOL VEYON_CORE_EXPORT enablePrivilege( const QString& privilegeName, bool enable );
	HWND VEYON_CORE_EXPORT getHWNDForWidget( const QWidget* widget );
#endif

	void VEYON_CORE_EXPORT activateWindow( QWidget * _window );

}

#endif
