/*
 * main.cpp - main file for iTALC Feature Worker
 *
 * Copyright (c) 2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QApplication>

#include "ItalcCore.h"
#include "Logger.h"
#include "BuiltinFeatures.h"
#include "FeatureManager.h"
#include "FeatureWorkerManagerConnection.h"
#include "PluginManager.h"


int main( int argc, char **argv )
{
	QApplication app( argc, argv );

	if( app.arguments().count() < 2 )
	{
		qFatal( "Not enough arguments (feature)" );
	}

	ItalcCore::init();

	PluginManager pluginManager;
	BuiltinFeatures builtinFeatures( pluginManager );
	FeatureManager featureManager( pluginManager );

	const Feature* workerFeature = nullptr;

	for( const auto& feature : featureManager.features() )
	{
		if( feature.uid() == app.arguments().last() )
		{
			workerFeature = &feature;
		}
	}

	if( workerFeature == nullptr )
	{
		qFatal( "Could not find specified feature" );
	}

	QString pluginName = pluginManager.pluginName( featureManager.pluginUid( *workerFeature ) );

	FeatureWorkerManagerConnection featureWorkerManagerConnection( featureManager, workerFeature->uid() );

	Logger l( "ItalcFeature" + pluginName );

	qInfo( "Exec" );

	return app.exec();
}
