/*
 * Filesystem.h - filesystem related query and manipulation functions
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "VeyonCore.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT Filesystem
{
public:
	QString expandPath( QString path ) const;
	QString shrinkPath( QString path ) const;
	bool ensurePathExists( const QString &path ) const;

	QString privateKeyPath( const QString& name ) const;
	QString publicKeyPath( const QString& name ) const;

	QString serverFilePath() const;
	QString workerFilePath() const;

	QString certificatePath() const;
	QString certificatePath( const QString& name ) const;

};

#endif
