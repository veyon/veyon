/*
 * VariantStream.h - read/write QVariant objects to/from QIODevice
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

#ifndef VARIANT_STREAM_H
#define VARIANT_STREAM_H

#include <QDataStream>

#include "VeyonCore.h"

class VEYON_CORE_EXPORT VariantStream
{
public:
	VariantStream( QIODevice* ioDevice );

	QVariant read();

	void write( const QVariant& v );

private:
	QDataStream m_dataStream;

} ;

#endif
