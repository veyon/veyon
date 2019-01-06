/*
 * Configuration/LocalStore.h - LocalStore class
 *
 * Copyright (c) 2009-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef CONFIGURATION_LOCAL_STORE_H
#define CONFIGURATION_LOCAL_STORE_H

#include "Configuration/Store.h"

class QSettings;

namespace Configuration
{

// clazy:excludeall=copyable-polymorphic

class VEYON_CORE_EXPORT LocalStore : public Store
{
public:
	LocalStore( Scope scope );

	void load( Object *obj ) override;
	void flush( const Object *obj ) override;
	bool isWritable() const override;
	void clear() override;

	QSettings *createSettingsObject() const;

} ;

}

#endif
