// SPDX-FileCopyrightText: 2017-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>
#include <QIcon>

#include "Feature.h"
#include "VeyonWorker.h"


int main(int argc, char** argv)
{
	VeyonCore::setupApplicationParameters();

	QApplication app(argc, argv);
	app.setWindowIcon(QIcon(QStringLiteral(":/core/icon64.png")));

	const auto arguments = app.arguments();

	if (arguments.count() < 2)
	{
		qFatal("Not enough arguments (feature)");
	}

	const auto featureUid = Feature::Uid{arguments[1]};
	if (featureUid.isNull())
	{
		qFatal("Invalid feature UID given");
	}

	VeyonWorker worker(featureUid);

	return worker.core().exec();
}
