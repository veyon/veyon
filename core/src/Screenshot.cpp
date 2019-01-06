/*
 *  Screenshot.cpp - class representing a screenshot
 *
 *  Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - http://veyon.io
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

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QMessageBox>
#include <QPainter>

#include "Screenshot.h"
#include "VeyonConfiguration.h"
#include "Computer.h"
#include "ComputerControlInterface.h"
#include "Filesystem.h"

Screenshot::Screenshot( const QString &fileName, QObject* parent ) :
	QObject( parent ),
	m_fileName( fileName ),
	m_image()
{
	if( !m_fileName.isEmpty() && QFileInfo( m_fileName ).isFile() )
	{
		m_image.load( m_fileName );
	}
}




void Screenshot::take( ComputerControlInterface::Pointer computerControlInterface )
{
	QString u = computerControlInterface->user();
	if( u.isEmpty() )
	{
		u = tr( "unknown" );
	}
	if( !u.contains( '(' ) )
	{
		u = QString( QStringLiteral( "%1 (%2)" ) ).arg( u, u );
	}

	// construct text
	QString txt = u + "@" + computerControlInterface->computer().hostAddress() + " " +
			QDate( QDate::currentDate() ).toString( Qt::ISODate ) +
			" " + QTime( QTime::currentTime() ).
							toString( Qt::ISODate );
	const QString dir = VeyonCore::filesystem().expandPath( VeyonCore::config().screenshotDirectory() );
	if( VeyonCore::filesystem().ensurePathExists( dir ) == false )
	{
		QString msg = tr( "Could not take a screenshot as directory %1 "
								"doesn't exist and couldn't be "
								"created." ).arg( dir );
		qCritical() << msg.toUtf8().constData();
		if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
		{
			QMessageBox::critical( nullptr, tr( "Screenshot" ), msg );
		}

		return;
	}

	// construct filename
	m_fileName =  QString( QStringLiteral( "_%1_%2_%3.png" ) ).arg( computerControlInterface->computer().hostAddress(),
						QDate( QDate::currentDate() ).toString( Qt::ISODate ),
						QTime( QTime::currentTime() ).toString( Qt::ISODate ) ).
					replace( ':', '-' );

	m_fileName = dir + QDir::separator() +
					u.section( '(', 1, 1 ).section( ')', 0, 0 ) + m_fileName;

	const int FONT_SIZE = 14;
	const int RECT_MARGIN = 10;
	const int RECT_INNER_MARGIN = 5;

	m_image = QImage( computerControlInterface->screen() );

	QPixmap icon( QStringLiteral( ":/resources/icon16.png" ) );

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




QString Screenshot::user() const
{
	return QFileInfo( fileName() ).fileName().section( '_', 0, 0 );
}




QString Screenshot::host() const
{
	return fileName().section( '_', 1, 1 );
}




QString Screenshot::date() const
{
	return QDate::fromString( fileName().section( '_', 2, 2 ),
										Qt::ISODate ).toString( Qt::LocalDate );
}




QString Screenshot::time() const
{
	return fileName().section( '_', 3, 3 ).section( '.', 0, 0 ).replace( '-', ':' );
}


