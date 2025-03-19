/*
 * VariantStream.cpp - read/write QVariant objects to/from QIODevice
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QIODevice>
#include <QUuid>
#include <QVariant>

#include "VariantStream.h"

VariantStream::VariantStream( QIODevice* ioDevice ) :
	m_dataStream( ioDevice )
{
	m_dataStream.setVersion( QDataStream::Qt_5_5 );
}



QVariant VariantStream::read() // Flawfinder: ignore
{
	QVariant v;

	m_dataStream.startTransaction();
	const auto isValid = checkVariant(0);
	m_dataStream.rollbackTransaction();

	if (isValid == false)
	{
		return {};
	}

	m_dataStream >> v;

	if( v.isValid() == false || v.isNull() )
	{
		vWarning() << "none or invalid data read";
	}

	return v;
}



void VariantStream::write( const QVariant& v )
{
	m_dataStream << v;
}



bool VariantStream::checkBool()
{
	bool b;
	m_dataStream >> b;
	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkByteArray()
{
	const auto pos = m_dataStream.device()->pos();

	quint32 len;
	m_dataStream >> len;

	// null array?
	if (len == 0xffffffff)
	{
		return true;
	}

	if (len > MaxByteArraySize)
	{
		vDebug() << "byte array too big";
		return false;
	}

	m_dataStream.device()->seek(pos);

	QByteArray s;
	m_dataStream >> s;

	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkInt()
{
	int i;
	m_dataStream >> i;
	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkLong()
{
	qlonglong i;
	m_dataStream >> i;
	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkRect()
{
	qint32 i;
	m_dataStream >> i >> i >> i >> i;
	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkString()
{
	const auto pos = m_dataStream.device()->pos();

	quint32 len;
	m_dataStream >> len;

	// null string?
	if (len == 0xffffffff)
	{
		return true;
	}

	if (len > MaxStringSize)
	{
		vDebug() << "string too long";
		return false;
	}

	m_dataStream.device()->seek(pos);

	QString s;
	m_dataStream >> s;

	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkStringList()
{
	quint32 n;
	m_dataStream >> n;

	if (n > MaxContainerSize)
	{
		vDebug() << "QStringList has too many elements";
		return false;
	}

	for (quint32 i = 0; i < n; ++i)
	{
		if (checkString() == false)
		{
			return false;
		}
	}

	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkUuid()
{
	QUuid uuid;
	m_dataStream >> uuid;
	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkVariant(int depth)
{
	if (depth > MaxCheckRecursionDepth)
	{
		vDebug() << "max recursion depth reached";
		return false;
	}

	quint32 typeId = 0;
	m_dataStream >> typeId;

	quint8 isNull = false;
	m_dataStream >> isNull;

	switch(typeId)
	{
	case QMetaType::Bool: return checkBool();
	case QMetaType::QByteArray: return checkByteArray();
	case QMetaType::Int: return checkInt();
	case QMetaType::LongLong: return checkLong();
	case QMetaType::QRect: return checkRect();
	case QMetaType::QString: return checkString();
	case QMetaType::QStringList: return checkStringList();
	case QMetaType::QUuid: return checkUuid();
	case QMetaType::QVariantList: return checkVariantList(depth);
	case QMetaType::QVariantMap: return checkVariantMap(depth);
	default:
		vDebug() << "invalid type" << typeId;
		return false;
	}

	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkVariantList(int depth)
{
	quint32 n;
	m_dataStream >> n;

	if (n > MaxContainerSize)
	{
		vDebug() << "QVariantList has too many elements";
		return false;
	}

	for (quint32 i = 0; i < n; ++i)
	{
		if (checkVariant(depth+1) == false)
		{
			return false;
		}
	}

	return m_dataStream.status() == QDataStream::Status::Ok;
}



bool VariantStream::checkVariantMap(int depth)
{
	quint32 n;
	m_dataStream >> n;

	if (n > MaxContainerSize)
	{
		vDebug() << "QVariantMap has too many elements";
		return false;
	}

	for (quint32 i = 0; i < n; ++i)
	{
		if (checkString() == false ||
			checkVariant(depth+1) == false)
		{
			return false;
		}
	}

	return m_dataStream.status() == QDataStream::Status::Ok;
}
