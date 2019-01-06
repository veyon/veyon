/*
 * ConfigurationPage.h - base class for all configuration pages
 *
 * Copyright (c) 2016-2019 Tobias Junghans <tobydox@veyon.io>
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

#ifndef CONFIGURATION_PAGE_H
#define CONFIGURATION_PAGE_H

#include "VeyonCore.h"

#include <QWidget>

class VEYON_CORE_EXPORT ConfigurationPage : public QWidget
{
	Q_OBJECT
public:
	ConfigurationPage( QWidget* parent = nullptr );
	~ConfigurationPage() override;

	virtual void resetWidgets() = 0;
	virtual void connectWidgetsToProperties() = 0;
	virtual void applyConfiguration() = 0;

};

#endif // CONFIGURATION_PAGE_H
