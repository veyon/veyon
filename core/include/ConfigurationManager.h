/*
 * ConfigurationManager.h - class for managing Veyon's configuration
 *
 * Copyright (c) 2010-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include "VeyonCore.h"

class VeyonConfiguration;

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT ConfigurationManager : public QObject
{
	Q_OBJECT
public:
	ConfigurationManager( QObject* parent = nullptr );

	bool clearConfiguration();
	bool applyConfiguration();
	bool saveConfiguration();

	const QString& errorString() const
	{
		return m_errorString;
	}

private:
	VeyonConfiguration& m_configuration;
	QString m_errorString;

} ;
