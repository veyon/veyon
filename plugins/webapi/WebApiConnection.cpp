/*
 * WebApiConnection.cpp - implementation of WebApiConnection class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QImageWriter>
#include <QTimer>

#include "ComputerControlInterface.h"
#include "WebApiConnection.h"


WebApiConnection::WebApiConnection( const QString& hostAddress ) :
	m_controlInterface( ComputerControlInterface::Pointer::create( Computer( {}, hostAddress, hostAddress ) ) ),
	m_idleTimer( new QTimer ),
	m_lifetimeTimer( new QTimer )
{
}



WebApiConnection::~WebApiConnection()
{
	m_framebufferEncoder.waitForFinished();

	m_idleTimer->deleteLater();
	m_lifetimeTimer->deleteLater();

	m_controlInterface->stop();
}



QSize WebApiConnection::scaledFramebufferSize( int width, int height ) const
{
	const auto screenSize = m_controlInterface->screen().size();
	QSize scaledSize{};

	if( width > 0 && height > 0 )
	{
		scaledSize = { width, height };
	}
	else if( width > 0 )
	{
		scaledSize = screenSize.scaled( width, screenSize.height(), Qt::KeepAspectRatio );
	}
	else if( height > 0 )
	{
		scaledSize = screenSize.scaled( screenSize.width(), height, Qt::KeepAspectRatio );
	}

	return scaledSize;
}



QByteArray WebApiConnection::encodedFramebufferData( QSize size, const QByteArray& format, int compression, int quality )
{
	if( m_lastFramebufferRequestTimer.isValid() )
	{
		m_lastFramebufferRequestInterval = ( m_lastFramebufferRequestInterval + m_lastFramebufferRequestTimer.restart() ) / 2;
	}
	else
	{
		m_lastFramebufferRequestTimer.start();
	}

	m_framebufferEncoder.waitForFinished();

	if( format != m_imageFormat ||
		compression != m_imageCompression ||
		quality != m_imageQuality ||
		size != m_imageSize ||
		m_framebufferEncoder.isCanceled() ||
		m_framebufferEncoder.result().expired() )
	{
		m_imageFormat = format;
		m_imageCompression = compression;
		m_imageQuality = quality;
		m_imageSize = size;

		runFramebufferEncoder();

		m_framebufferEncoder.waitForFinished();
	}

	const auto result = m_framebufferEncoder.result();

	m_encodingError = result.errorString;

	const auto preencodeInterval = m_lastFramebufferRequestInterval -
								   m_framebufferEncodingTime * 125 / 100;

	if( preencodeInterval >= MinimumPreencodeInterval )
	{
		QTimer::singleShot( preencodeInterval,
							m_controlInterface.data(), [this]() {
								lock();
								runFramebufferEncoder();
								unlock();
							} );
	}
	else
	{
		m_framebufferEncoder = {};
	}


	return result.imageData;
}



void WebApiConnection::runFramebufferEncoder()
{
	m_framebufferEncoder = QtConcurrent::run( [this]() {

		QElapsedTimer encodingTimer;
		encodingTimer.start();

		EncodingResult result;
		QBuffer dataBuffer( &result.imageData );
		dataBuffer.open( QBuffer::WriteOnly );
		QImageWriter imageWriter( &dataBuffer, m_imageFormat );

		if( m_imageCompression > 0 )
		{
			static constexpr auto QtPngCompressionLevelFactor = 11;

			imageWriter.setCompression( m_imageCompression * QtPngCompressionLevelFactor );
		}

		if( m_imageQuality > 0 )
		{
			imageWriter.setQuality( m_imageQuality );
		}

		if( m_imageSize != m_controlInterface->scaledScreenSize() )
		{
			m_controlInterface->setScaledScreenSize( m_imageSize );
		}

		const auto writeResult = imageWriter.write(
			m_imageSize.isEmpty() ?
								 controlInterface()->screen() :
								 controlInterface()->scaledScreen() );

		dataBuffer.close();

		if( writeResult == false )
		{
			result.imageData = {};
			result.errorString = imageWriter.errorString();
		}

		m_framebufferEncodingTime = ( m_framebufferEncodingTime + encodingTimer.elapsed() ) / 2;

		return result;
	} );
}
