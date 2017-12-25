/*
 * VeyonServiceControl.h - class for controlling the Veyon service
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

#ifndef VEYON_SERVICE_CONTROL_H
#define VEYON_SERVICE_CONTROL_H

#include "ServiceControl.h"

// clazy:excludeall=ctor-missing-parent-argument

class VEYON_CORE_EXPORT VeyonServiceControl : public ServiceControl
{
	Q_OBJECT
public:
	VeyonServiceControl( QWidget* parent = nullptr );

	bool setAutostart( bool enabled );

	static QString name();
	static QString filePath();
	static QString displayName();
};

#endif // VEYON_SERVICE_CONTROL_H
