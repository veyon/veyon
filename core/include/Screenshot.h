/*
 *  Screenshot.h - class representing a screenshot
 *
 *  Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "VeyonCore.h"

#include <QImage>
#include <QPixmap>

class ComputerControlInterface;

class VEYON_CORE_EXPORT Screenshot : public QObject
{
	Q_OBJECT
public:
	Screenshot( const QString &fileName = QString(), QObject* parent = nullptr );

	void take( const ComputerControlInterface& computerControlInterface );

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

