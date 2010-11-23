/*
 *  Snapshot.h - class representing a screen snapshot
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

#ifndef _SNAPSHOT_H
#define _SNAPSHOT_H

#include <QtGui/QImage>
#include <QtGui/QPixmap>

class ItalcVncConnection;

class Snapshot : public QObject
{
	Q_OBJECT
public:
	Snapshot( const QString &fileName = QString() );

	void take( ItalcVncConnection *vncConn, const QString &user );

	bool isValid() const
	{
		return !fileName().isEmpty() && !image().isNull();
	}

	const QString &fileName() const
	{
		return m_fileName;
	}

	const QImage &image() const
	{
		return m_image;
	}

	QPixmap pixmap() const
	{
		return QPixmap::fromImage( image() );
	}

	QString user() const;
	QString host() const;
	QString date() const;
	QString time() const;


private:
	QString m_fileName;
	QImage m_image;

} ;

#endif

