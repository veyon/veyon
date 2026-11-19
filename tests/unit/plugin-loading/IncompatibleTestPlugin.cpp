// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QObject>

// not defined anywhere, like a function which was removed from or changed in veyon-core
extern "C" void veyonMissingSymbolForTest();

// plugin with an unresolved symbol, like a plugin built for an older Veyon version
class IncompatibleTestPlugin : public QObject
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.PluginLoadingTest")
public:
	Q_INVOKABLE void callMissingSymbol()
	{
		veyonMissingSymbolForTest();
	}
};

#include "IncompatibleTestPlugin.moc"
