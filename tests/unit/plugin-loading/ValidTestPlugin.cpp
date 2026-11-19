// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QObject>

// minimal plugin which loads fine
class ValidTestPlugin : public QObject
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.PluginLoadingTest")
};

#include "ValidTestPlugin.moc"
