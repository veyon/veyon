/*
 *  Snapshot.cpp - class representing a screen snapshot
 *
 *  Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 *  This file is part of iTALC - http://italc.sourceforge.net
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QApplication>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>

#include "Snapshot.h"
#include "ItalcConfiguration.h"
#include "ItalcVncConnection.h"
#include "LocalSystem.h"
#include "Logger.h"


Snapshot::Snapshot( const QString &fileName ) :
	m_fileName( fileName ),
	m_image()
{
	if( !m_fileName.isEmpty() && QFileInfo( m_fileName ).isFile() )
	{
		m_image.load( m_fileName );
	}
}




void Snapshot::take( ItalcVncConnection *vncConn, const QString &user )
{
	QString u = user;
	if( u.isEmpty() )
	{
		u = tr( "unknown" );
	}
	if( !u.contains( '(' ) )
	{
		u = QString( "%1 (%2)" ).arg( u ).arg( u );
	}

	// construct text
	QString txt = u + "@" + vncConn->host() + " " +
			QDate( QDate::currentDate() ).toString( Qt::ISODate ) +
			" " + QTime( QTime::currentTime() ).
							toString( Qt::ISODate );
	const QString dir = LocalSystem::Path::expand(
									ItalcCore::config->snapshotDirectory() );
	if( !LocalSystem::Path::ensurePathExists( dir ) )
	{
		QString msg = tr( "Could not take a snapshot as directory %1 "
								"doesn't exist and couldn't be "
								"created." ).arg( dir );
		qCritical() << msg.toUtf8().constData();
		if( QApplication::type() != QApplication::Tty )
		{
			QMessageBox::critical( NULL, tr( "Snapshot" ), msg );
		}

		return;
	}

	// construct filename
	m_fileName =  QString( "_%1_%2_%3.png" ).
						arg( vncConn->host() ).
						arg( QDate( QDate::currentDate() ).toString( Qt::ISODate ) ).
						arg( QTime( QTime::currentTime() ).toString( Qt::ISODate ) ).
					replace( ':', '-' );

	m_fileName = dir + QDir::separator() +
					u.section( '(', 1, 1 ).section( ')', 0, 0 ) + m_fileName;

	const int FONT_SIZE = 14;
	const int RECT_MARGIN = 10;
	const int RECT_INNER_MARGIN = 5;

	m_image = QImage( vncConn->image() );

	QPixmap icon( QPixmap( ":/resources/icon16.png" ) );

	QPainter p( &m_image );
	QFont fnt = p.font();
	fnt.setPointSize( FONT_SIZE );
	fnt.setBold( true );
	p.setFont( fnt );

	QFontMetrics fm( p.font() );

	const int rx = RECT_MARGIN;
	const int ry = m_image.height() - RECT_MARGIN - 2 * RECT_INNER_MARGIN - FONT_SIZE;
	const int rw = RECT_MARGIN + 4 * RECT_INNER_MARGIN +
					fm.size( Qt::TextSingleLine, txt ).width() + icon.width();
	const int rh = 2 * RECT_INNER_MARGIN + FONT_SIZE;
	const int ix = rx + RECT_INNER_MARGIN + 1;
	const int iy = ry + RECT_INNER_MARGIN - 2;
	const int tx = ix + icon.width() + 2 * RECT_INNER_MARGIN;
	const int ty = ry + RECT_INNER_MARGIN + FONT_SIZE - 2;

	p.fillRect( rx, ry, rw, rh, QColor( 255, 255, 255, 160 ) );
	p.drawPixmap( ix, iy, icon );
	p.drawText( tx, ty, txt );

	m_image.save( m_fileName, "PNG", 50 );
}




QString Snapshot::user() const
{
	return QFileInfo( fileName() ).fileName().section( '_', 0, 0 );
}




QString Snapshot::host() const
{
	return fileName().section( '_', 1, 1 );
}




QString Snapshot::date() const
{
	return QDate::fromString( fileName().section( '_', 2, 2 ),
										Qt::ISODate ).toString( Qt::LocalDate );
}




QString Snapshot::time() const
{
	return fileName().section( '_', 3, 3 ).section( '.', 0, 0 ).replace( '-', ':' );
}


