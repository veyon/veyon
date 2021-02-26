/*
 * FileReadThread.cpp - implementation of FileReadThread class
 *
 * Copyright (c) 2018-2021 Tobias Junghans <tobydox@veyon.io>
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

#include "FileReadThread.h"


FileReadThread::FileReadThread( const QString& fileName, QObject* parent ) :
	QObject( parent ),
	m_mutex(),
	m_thread( new QThread ),
	m_file( nullptr ),
	m_currentChunk(),
	m_timer( new QTimer ),
	m_fileName( fileName ),
	m_chunkReady( false ),
	m_filePos( 0 ),
	m_fileSize( 0 )
{
	m_timer->moveToThread( m_thread );
	m_thread->start();

	connect( m_thread, &QThread::finished, m_timer, &QObject::deleteLater );
	connect( m_thread, &QThread::finished, m_thread, &QObject::deleteLater );
}



FileReadThread::~FileReadThread()
{
	m_thread->quit();
}



bool FileReadThread::start()
{
	if( QFile( m_fileName ).open( QFile::ReadOnly ) == false )
	{
		return false;
	}

	m_timer->singleShot( 0, this, [this]() {
		m_file = new QFile( m_fileName );
		m_file->open( QFile::ReadOnly );
		connect( m_thread, &QThread::finished, m_file, &QObject::deleteLater );

		m_mutex.lock();
		m_filePos = 0;
		m_fileSize = m_file->size();
		m_mutex.unlock();
	} );

	return true;
}



QByteArray FileReadThread::currentChunk()
{
	QMutexLocker lock( &m_mutex );
	return m_currentChunk;
}



void FileReadThread::readNextChunk( qint64 chunkSize )
{
	m_mutex.lock();
	m_chunkReady = false;
	m_mutex.unlock();

	m_timer->singleShot( 0, this, [this, chunkSize]() {
		if( m_file )
		{
			const auto chunk = m_file->read( chunkSize );
			m_mutex.lock();
			m_currentChunk = chunk;
			m_chunkReady = true;
			m_filePos = m_file->pos();
			m_mutex.unlock();
		}
	} );
}



bool FileReadThread::isChunkReady()
{
	QMutexLocker lock( &m_mutex );
	return m_chunkReady;
}



bool FileReadThread::atEnd()
{
	QMutexLocker lock( &m_mutex );
	return m_filePos >= m_fileSize;
}



int FileReadThread::progress()
{
	QMutexLocker lock( &m_mutex );
	return m_fileSize > 0 ? static_cast<int>( m_filePos * 100 / m_fileSize ) : 0;
}
