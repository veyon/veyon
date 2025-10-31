#pragma once

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"
#include "MacVncConfiguration.h"

class BuiltinMacVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.BuiltinMacVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	explicit BuiltinMacVncServer( QObject* parent = nullptr );

	Plugin::Uid uid() const override
	{
		return Plugin::Uid{ QStringLiteral("5e9c0449-5bb7-4d1d-b793-71462a798a59") };
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "BuiltinMacVncServer" );
	}

	QString description() const override
	{
		return tr( "Builtin VNC server (macOS)" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QStringList supportedSessionTypes() const override
	{
		return {};
	}

	QWidget* configurationWidget() override;

	void prepareServer() override;

	bool runServer( int serverPort, const Password& password ) override;

	int configuredServerPort() override
	{
		return -1;
	}

	Password configuredPassword() override
	{
		return {};
	}

private:
	MacVncConfiguration m_configuration;
};
