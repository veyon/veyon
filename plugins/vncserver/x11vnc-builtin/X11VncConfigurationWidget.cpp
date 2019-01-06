/*
 * X11VncConfigurationWidget.h - implementation of the X11VncConfigurationWidget class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
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

#include "Configuration/UiMapping.h"
#include "X11VncConfiguration.h"
#include "X11VncConfigurationWidget.h"

#include "ui_X11VncConfigurationWidget.h"

X11VncConfigurationWidget::X11VncConfigurationWidget( X11VncConfiguration& configuration, QWidget* parent  ) :
	QWidget( parent ),
	ui( new Ui::X11VncConfigurationWidget ),
	m_configuration( configuration )
{
	ui->setupUi( this );

	FOREACH_X11VNC_CONFIG_PROPERTY(INIT_WIDGET_FROM_PROPERTY);
	FOREACH_X11VNC_CONFIG_PROPERTY(CONNECT_WIDGET_TO_PROPERTY);
}



X11VncConfigurationWidget::~X11VncConfigurationWidget()
{
	delete ui;
}
