/*
 * FileSystemBrowser.h - a wrapper class for easily browsing the file system
 *
 * Copyright (c) 2010-2025 Tobias Junghans <tobydox@veyon.io>
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

#include "VeyonCore.h"

class QLineEdit;

class VEYON_CORE_EXPORT FileSystemBrowser
{
public:
	enum BrowseModes
	{
		ExistingDirectory,
		ExistingFile,
		SaveFile,
		NumBrowseModes
	} ;
	using BrowseMode = BrowseModes;

	explicit FileSystemBrowser(BrowseMode m, QWidget* parent) :
		m_parent(parent),
		m_browseMode( m ),
		m_expandPath( true ),
		m_shrinkPath( true )
	{
	}

	void setExpandPath( bool enabled )
	{
		m_expandPath = enabled;
	}

	void setShrinkPath( bool enabled )
	{
		m_shrinkPath = enabled;
	}

	QString exec( const QString &path, const QString &title = {},
					const QString &filter = {} );
	void exec( QLineEdit *lineEdit, const QString &title = {},
					const QString &filter = {} );


private:
	QWidget* m_parent;
	BrowseMode m_browseMode;
	bool m_expandPath;
	bool m_shrinkPath;

} ;
