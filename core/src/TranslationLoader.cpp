/*
 * TranslationLoader.cpp - implementation of TranslationLoader class
 *
 * Copyright (c) 2020-2021 Tobias Junghans <tobydox@veyon.io>
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

#include <QCoreApplication>
#include <QRegularExpression>
#include <QTranslator>

#include "TranslationLoader.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


TranslationLoader::TranslationLoader( const QString& resourceName )
{
	load( resourceName );
}



QLocale TranslationLoader::load( const QString& resourceName )
{
	QLocale configuredLocale( QLocale::C );

	const auto configuredLocaleMatch = QRegularExpression{ QStringLiteral( "[^(]*\\(([^)]*)\\)") }
										   .match( VeyonCore::config().uiLanguage() );
	if( configuredLocaleMatch.hasMatch() )
	{
		configuredLocale = QLocale( configuredLocaleMatch.captured( 1 ) );
	}

	if( configuredLocale.language() != QLocale::English &&
		VeyonCore::instance()->findChild<QTranslator *>( resourceName ) == nullptr )
	{
		const auto translationsDirectory = resourceName.startsWith( QLatin1String("qt") )
											  ? VeyonCore::qtTranslationsDirectory()
											  : VeyonCore::translationsDirectory();

		auto translator = new QTranslator( VeyonCore::instance() );
		translator->setObjectName( resourceName );

		if( configuredLocale == QLocale::C ||
			translator->load( QStringLiteral( "%1_%2.qm" ).arg( resourceName, configuredLocale.name() ),
							  translationsDirectory ) == false )
		{
			configuredLocale = QLocale::system(); // Flawfinder: ignore

			if( translator->load( QStringLiteral( "%1_%2.qm" ).arg( resourceName, configuredLocale.name() ),
								  translationsDirectory ) == false )
			{
				(void) translator->load( QStringLiteral( "%1_%2.qm" ).arg( resourceName, configuredLocale.language() ),
										 translationsDirectory() );
			}
		}

		QLocale::setDefault( configuredLocale );

		QCoreApplication::installTranslator( translator );
	}

	return configuredLocale;
}
