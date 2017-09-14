/*
 * FileSystemBrowser.cpp - a wrapper class for easily browsing the file system
 *
 * Copyright (c) 2010-2013 Tobias Junghans <tobydox@users.sf.net>
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

#include <QFileDialog>
#include <QLineEdit>

#include "FileSystemBrowser.h"
#include "LocalSystem.h"


QString FileSystemBrowser::exec( const QString &path,
									const QString &title,
									const QString &filter )
{
	QString browsePath = path;
	if( m_expandPath )
	{
		browsePath = LocalSystem::Path::expand( browsePath );
	}

	switch( m_browseMode )
	{
		case ExistingDirectory:
			if( !QFileInfo( browsePath ).isDir() )
			{
				browsePath = QDir::homePath();
			}
			break;
		case ExistingFile:
		case SaveFile:
			if( QFileInfo( browsePath ).isFile() )
			{
				browsePath = QFileInfo( browsePath ).absolutePath();
			}
			else
			{
				browsePath = QDir::homePath();
			}
			break;

		default: break;
	}

	QString chosenPath;
	switch( m_browseMode )
	{
		case ExistingDirectory:
			chosenPath = QFileDialog::getExistingDirectory( nullptr, title,
								browsePath,
		                		QFileDialog::ShowDirsOnly |
									QFileDialog::DontResolveSymlinks );
			break;
		case ExistingFile:
			chosenPath = QFileDialog::getOpenFileName( nullptr, title,
														browsePath, filter );
			break;
		case SaveFile:
			chosenPath = QFileDialog::getSaveFileName( nullptr, title,
														browsePath, filter );
			break;

		default: break;
	}

	if( !chosenPath.isEmpty() )
	{
		if( m_shrinkPath )
		{
			return LocalSystem::Path::shrink( chosenPath );
		}
		return chosenPath;
	}

	return path;
}



void FileSystemBrowser::exec( QLineEdit *lineEdit, const QString &title,
								const QString & filter )
{
	lineEdit->setText( exec( lineEdit->text(), title, filter ) );
}

