// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QObject>

// minimal plugin which loads fine if built for this Veyon version
class ValidTestPlugin : public QObject
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.PluginLoadingTest")
public:
	ValidTestPlugin()
	{
		// lets the test check whether the plugin has been instantiated
		qputenv("VEYON_TEST_PLUGIN_INSTANTIATED_" TEST_PLUGIN_NAME, QByteArrayLiteral("1"));
	}
};

#include "ValidTestPlugin.moc"
