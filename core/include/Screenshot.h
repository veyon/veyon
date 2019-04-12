/*
 *  Screenshot.h - class representing a screenshot
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

#pragma once

#include "ComputerControlInterface.h"
#include "VeyonCore.h"

#include <QDate>
#include <QImage>

class ComputerControlInterface;

class VEYON_CORE_EXPORT Screenshot : public QObject
{
	Q_OBJECT
public:
	Screenshot( const QString &fileName = QString(), QObject* parent = nullptr );

	void take( const ComputerControlInterface::Pointer& computerControlInterface );

	bool isValid() const
	{
		return !fileName().isEmpty() && !image().isNull();
	}

	const QString& fileName() const
	{
		return m_fileName;
	}

	void setImage( const QImage& image )
	{
		m_image = image;
	}

	const QImage& image() const
	{
		return m_image;
	}

	static QString constructFileName( const QString& user, const QString& hostAddress,
									  const QDate& date = QDate::currentDate(),
									  const QTime& time = QTime::currentTime() );

	QString user() const;
	QString host() const;
	QString date() const;
	QString time() const;


private:
	QString m_fileName;
	QImage m_image;

} ;

