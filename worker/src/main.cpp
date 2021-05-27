/*
 * main.cpp - main file for Veyon Feature Worker
 *
 * Copyright (c) 2017-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>

#include "VeyonWorker.h"


int main( int argc, char **argv )
{
	VeyonCore::setupApplicationParameters();

	QApplication app( argc, argv );
	app.setWindowIcon( QIcon( QStringLiteral(":/core/icon64.png") ) );

	const auto arguments = app.arguments();

	if( arguments.count() < 2 )
	{
		qFatal( "Not enough arguments (feature)" );
	}

	const auto featureUid = arguments[1];
	if( QUuid( featureUid ).isNull() )
	{
		qFatal( "Invalid feature UID given" );
	}

	VeyonWorker worker( featureUid );

	return worker.core().exec();
}
