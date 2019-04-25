/*
 *  Screenshot.cpp - class representing a screenshot
 *
 *  Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
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



void Screenshot::take( const ComputerControlInterface::Pointer& computerControlInterface )
{
	auto userLogin = computerControlInterface->userLoginName();
	if( userLogin.isEmpty() )
	{
		userLogin = tr( "unknown" );
	}

	const auto dir = VeyonCore::filesystem().expandPath( VeyonCore::config().screenshotDirectory() );

	if( VeyonCore::filesystem().ensurePathExists( dir ) == false )
	{
		const auto msg = tr( "Could not take a screenshot as directory %1 doesn't exist and couldn't be created." ).arg( dir );
		vCritical() << msg.toUtf8().constData();
		if( qobject_cast<QApplication *>( QCoreApplication::instance() ) )
		{
			QMessageBox::critical( nullptr, tr( "Screenshot" ), msg );
		}

		return;
	}

	// construct filename
	m_fileName = dir + QDir::separator() + constructFileName( userLogin, computerControlInterface->computer().hostAddress() );

	// construct caption
	auto user = userLogin;
	if( computerControlInterface->userFullName().isEmpty() == false )
	{
		user = QStringLiteral( "%1 (%2)" ).arg( userLogin, computerControlInterface->userFullName() );
	}

	const auto caption = QStringLiteral( "%1@%2 %3 %4" ).arg( user,
															  computerControlInterface->computer().hostAddress(),
															  QDate::currentDate().toString( Qt::ISODate ),
															  QTime::currentTime().toString( Qt::ISODate ) );

	const int FONT_SIZE = 14;
	const int RECT_MARGIN = 10;
	const int RECT_INNER_MARGIN = 5;

	m_image = QImage( computerControlInterface->screen() );

	QPixmap icon( QStringLiteral( ":/core/icon16.png" ) );

	QPainter p( &m_image );
	QFont fnt = p.font();
	fnt.setPointSize( FONT_SIZE );
	fnt.setBold( true );
	p.setFont( fnt );

	QFontMetrics fm( p.font() );

	const int rx = RECT_MARGIN;
	const int ry = m_image.height() - RECT_MARGIN - 2 * RECT_INNER_MARGIN - FONT_SIZE;
	const int rw = RECT_MARGIN + 4 * RECT_INNER_MARGIN +
					fm.size( Qt::TextSingleLine, caption ).width() + icon.width();
	const int rh = 2 * RECT_INNER_MARGIN + FONT_SIZE;
	const int ix = rx + RECT_INNER_MARGIN + 1;
	const int iy = ry + RECT_INNER_MARGIN - 2;
	const int tx = ix + icon.width() + 2 * RECT_INNER_MARGIN;
	const int ty = ry + RECT_INNER_MARGIN + FONT_SIZE - 2;

	p.fillRect( rx, ry, rw, rh, QColor( 255, 255, 255, 160 ) );
	p.drawPixmap( ix, iy, icon );
	p.drawText( tx, ty, caption );

	m_image.save( m_fileName, "PNG", 50 );
}



QString Screenshot::constructFileName( const QString& user, const QString& hostAddress,
									   const QDate& date, const QTime& time )
{
	return QStringLiteral( "%1_%2_%3_%4.png" ).arg( user,
													hostAddress,
													date.toString( Qt::ISODate ),
													time.toString( Qt::ISODate ) ).
			replace( QLatin1Char(':'), QLatin1Char('-') );
}



QString Screenshot::user() const
{
	return QFileInfo( fileName() ).fileName().section( QLatin1Char('_'), 0, 0 );
}



QString Screenshot::host() const
{
	return fileName().section( QLatin1Char('_'), 1, 1 );
}



QString Screenshot::date() const
{
	return QDate::fromString( fileName().section( QLatin1Char('_'), 2, 2 ),
										Qt::ISODate ).toString( Qt::LocalDate );
}



QString Screenshot::time() const
{
	return fileName().section( QLatin1Char('_'), 3, 3 ).section( QLatin1Char('.'), 0, 0 ).replace( QLatin1Char('-'), QLatin1Char(':') );
}
