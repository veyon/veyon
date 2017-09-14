/*
 * ConfigurationTestController.h - class for automated configuration tests
 *
 * Copyright (c) 2016 Tobias Junghans <tobydox@users.sf.net>
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

#ifndef CONFIGURATION_TEST_CONTROLLER_H
#define CONFIGURATION_TEST_CONTROLLER_H

#include <QStringList>

class ConfigurationTestController
{
public:
	ConfigurationTestController( const QStringList& testArguments );

	int run();


private:
	QString testArgument( int i ) const
	{
		return m_testArguments.value( i );
	}

	QStringList m_testArguments;

};

#endif // CONFIGURATION_TEST_CONTROLLER_H
