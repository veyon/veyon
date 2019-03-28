/*
 * DocumentationFigureCreator.cpp - helper for creating documentation figures
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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

#include "DocumentationFigureCreator.h"
#include "FeatureManager.h"
#include "MainToolBar.h"
#include "MainWindow.h"
#include "Plugin.h"
#include "PluginManager.h"
#include "ToolButton.h"
#include "VeyonConfiguration.h"


DocumentationFigureCreator::DocumentationFigureCreator() :
	QObject(),
	m_master( nullptr )
{
	m_master = new VeyonMaster;
}



void DocumentationFigureCreator::run()
{
	auto mainWindow = m_master->mainWindow();
	auto toolbar = mainWindow->findChild<MainToolBar *>();

	mainWindow->resize( 3000, 1000 );
	mainWindow->show();

	Plugin::Uid previousPluginUid;
	Feature const* previousFeature = nullptr;

	int x = -1;
	int w = 0;

	const QStringList separatedPluginFeatures( { QStringLiteral("a54ee018-42bf-4569-90c7-0d8470125ccf"),
												 QStringLiteral("80580500-2e59-4297-9e35-e53959b028cd")
											   } );

	const auto& features = m_master->features();

	for( const auto& feature : features )
	{
		auto btn = toolbar->findChild<ToolButton *>( feature.name() );
		const auto pluginUid = m_master->featureManager().pluginUid( feature );

		if( previousPluginUid.isNull() )
		{
			previousPluginUid = pluginUid;
			previousFeature = &feature;
			x = btn->x();
		}

		const auto separatedFeature = separatedPluginFeatures.contains( VeyonCore::formattedUuid( previousPluginUid ) );

		if( pluginUid != previousPluginUid || separatedFeature )
		{
			auto fileName = VeyonCore::pluginManager().pluginName( previousPluginUid );
			if( separatedFeature )
			{
				fileName = previousFeature->name();
			}
			createFigure( toolbar, QPoint( x-2, btn->y()-1 ), QSize( w, btn->height() + 2 ),
						  QStringLiteral("Feature%1.png").arg( fileName ) );
			previousPluginUid = pluginUid;
			x = btn->x();
			w = btn->width() + 4;
		}
		else
		{
			w += btn->width() + 2;
		}

		if( feature == features.last() )
		{
			auto fileName = VeyonCore::pluginManager().pluginName( pluginUid );
			if( separatedFeature )
			{
				fileName = feature.name();
			}
			createFigure( toolbar, QPoint( x-2, btn->y()-1 ), QSize( w, btn->height() + 2 ),
						  QStringLiteral("Feature%1.png").arg( fileName ) );
		}

		previousFeature = &feature;
	}
}



void DocumentationFigureCreator::createFigure(QWidget* widget, const QPoint& pos, const QSize& size, const QString& fileName)
{
	QPixmap pixmap( size );
	widget->render( &pixmap, QPoint(), QRegion( QRect( pos, size ) ) );
	pixmap.save( fileName );
	qCritical() << widget << pos << size << fileName;
}
