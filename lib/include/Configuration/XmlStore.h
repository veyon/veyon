/*
 * Configuration/XmlStore.h - XmlStore class
 *
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

#ifndef _CONFIGURATION_XML_STORE_H
#define _CONFIGURATION_XML_STORE_H

#include "Configuration/Store.h"


namespace Configuration
{

class XmlStore : public Store
{
public:
	XmlStore( Scope scope, const QString & file = QString() );

	virtual void load( Object *obj );
	virtual void flush( Object *obj );
	virtual bool isWritable() const;


private:
	QString configurationFilePath() const;

	QString m_file;

} ;

}

#endif
