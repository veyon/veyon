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

	const auto host = computerControlInterface->computer().hostAddress();
	const auto date = QDate::currentDate().toString( Qt::ISODate );
	const auto time = QTime::currentTime().toString( Qt::ISODate );

	const auto caption = QStringLiteral( "%1@%2 %3 %4" ).arg( user, host, date, time );

	m_image = computerControlInterface->screen();

	QPixmap icon( QStringLiteral( ":/core/icon16.png" ) );

	QPainter painter( &m_image );

	auto font = painter.font();
	font.setPointSize( 14 );
	font.setBold( true );
	painter.setFont( font );

	const QFontMetrics fontMetrics( painter.font() );
	const auto captionHeight = fontMetrics.boundingRect( caption ).height();

	const auto MARGIN = 14;
	const auto PADDING = 7;
	const QRect rect{ MARGIN,
				m_image.height() - MARGIN - 2 * PADDING - captionHeight,
				4 * PADDING + fontMetrics.horizontalAdvance( caption ) + icon.width(),
				2 * PADDING + captionHeight };
	const auto iconX = rect.x() + PADDING + 1;
	const auto iconY = rect.y() + ( rect.height() - icon.height() ) / 2;
	const auto textX = iconX + icon.width() + PADDING;
	const auto textY = rect.y() + PADDING + fontMetrics.ascent();

	painter.fillRect( rect, QColor( 255, 255, 255, 160 ) );
	painter.drawPixmap( iconX, iconY, icon );
	painter.drawText( textX, textY, caption );

	m_image.setText( QStringLiteral("User"), user );
	m_image.setText( QStringLiteral("Host"), host );
	m_image.setText( QStringLiteral("Date"), date );
	m_image.setText( QStringLiteral("Time"), time );

	m_image.save( m_fileName, "PNG", 50 );
}



QString Screenshot::constructFileName( const QString& user, const QString& hostAddress,
									   const QDate& date, const QTime& time )
{
	const auto userSimplified = VeyonCore::stripDomain( user ).toLower().remove(
				QRegularExpression( QStringLiteral("[^a-z0-9.]") ) );

	return QStringLiteral( "%1_%2_%3_%4.png" ).arg( userSimplified,
													hostAddress,
													date.toString( Qt::ISODate ),
													time.toString( Qt::ISODate ) ).
			replace( QLatin1Char(':'), QLatin1Char('-') );
}



QString Screenshot::user() const
{
	return property( QStringLiteral("User"), 0 );
}



QString Screenshot::host() const
{
	return property( QStringLiteral("Host"), 1 );
}



QString Screenshot::date() const
{
	return QDate::fromString( property( QStringLiteral("Date"), 2 ),
										Qt::ISODate ).toString( Qt::LocalDate );
}



QString Screenshot::time() const
{
	return property( QStringLiteral("Time"), 3 ).section( QLatin1Char('.'), 0, 0 ).replace( QLatin1Char('-'), QLatin1Char(':') );
}



QString Screenshot::property( const QString& key, int section ) const
{
	const auto embeddedProperty = m_image.text( key );
	if( embeddedProperty.isEmpty() )
	{
		return fileNameSection( section );
	}

	return embeddedProperty;
}



QString Screenshot::fileNameSection( int n ) const
{
	return QFileInfo( fileName() ).fileName().section( QLatin1Char('_'), n, n );
}
