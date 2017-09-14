/*
 * VariantArrayMessage.h - class for sending/receiving a variant array as message
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef VARIANT_ARRAY_MESSAGE_H
#define VARIANT_ARRAY_MESSAGE_H

#include <QBuffer>
#include <QVariant>

#include "VariantStream.h"

// clazy:excludeall=rule-of-three

class VEYON_CORE_EXPORT VariantArrayMessage
{
public:
	typedef quint32 MessageSize;

	VariantArrayMessage( QIODevice* ioDevice );

	bool send();

	bool isReadyForReceive();

	bool receive();

	QVariant read();

	VariantArrayMessage& write( const QVariant& v );

	QIODevice* ioDevice() const
	{
		return m_ioDevice;
	}

private:
	QBuffer m_buffer;
	VariantStream m_stream;
	QIODevice* m_ioDevice;

} ;

#endif // VARIANT_ARRAY_MESSAGE_H
