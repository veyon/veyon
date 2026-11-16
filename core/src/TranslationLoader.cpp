// SPDX-FileCopyrightText: 2020-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QCoreApplication>
#include <QRegularExpression>
#include <QTranslator>

#include "TranslationLoader.h"
#include "VeyonConfiguration.h"
#include "VeyonCore.h"


TranslationLoader::TranslationLoader(const QString& resourceName)
{
	load(resourceName);
}



bool TranslationLoader::load(const QString& resourceName)
{
	QLocale configuredLocale(QLocale::C);

	static const QRegularExpression configuredLocaleRX{QStringLiteral("[^(]*\\(([^)]*)\\)")};
	const auto configuredLocaleMatch = configuredLocaleRX.match(VeyonCore::config().uiLanguage());
	if (configuredLocaleMatch.hasMatch())
	{
		configuredLocale = QLocale(configuredLocaleMatch.captured(1));
	}

	if (configuredLocale.language() != QLocale::English &&
		VeyonCore::instance()->findChild<QTranslator*>(resourceName) == nullptr)
	{
		const auto translationsDirectory = resourceName.startsWith(QLatin1String("qt")) ? VeyonCore::qtTranslationsDirectory() : VeyonCore::translationsDirectory();

		auto translator = new QTranslator(VeyonCore::instance());
		translator->setObjectName(resourceName);

		if (configuredLocale == QLocale::C ||
			translator->load(QStringLiteral("%1_%2.qm").arg(resourceName, configuredLocale.name()),
							 translationsDirectory) == false)
		{
			configuredLocale = QLocale::system(); // Flawfinder: ignore

			if (translator->load(QStringLiteral("%1_%2.qm").arg(resourceName, configuredLocale.name()),
								 translationsDirectory) == false)
			{
				delete translator;
				return false;
			}
		}

		QLocale::setDefault(configuredLocale);

		QCoreApplication::installTranslator(translator);
	}

	return true;
}
