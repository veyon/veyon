#include <QByteArray>
#include <QThread>

#include "BuiltinMacVncServer.h"
#include "MacVncConfigurationWidget.h"
#include "VeyonConfiguration.h"

#include "macvnc.h"

BuiltinMacVncServer::BuiltinMacVncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



QWidget* BuiltinMacVncServer::configurationWidget()
{
	return new MacVncConfigurationWidget( m_configuration );
}



void BuiltinMacVncServer::prepareServer()
{
}



bool BuiltinMacVncServer::runServer( int serverPort, const Password& password )
{
	macvnc_options_t options;
	macvnc_default_options( &options );
	options.port = serverPort;

	const auto passwordBytes = password.toByteArray();
	options.password = passwordBytes.isEmpty() ? nullptr : passwordBytes.constData();

	options.view_only = m_configuration.viewOnly();
	options.prevent_dimming = m_configuration.preventDimming();
	options.prevent_sleep = m_configuration.preventSleep();
	options.display = m_configuration.displayIndex();

	vDebug() << "BuiltinMacVncServer::runServer(): About to call macvnc_start with port" << serverPort;

	char errorBuffer[512] = {0};
	if( macvnc_start( &options, errorBuffer, sizeof(errorBuffer) ) == false )
	{
		if( errorBuffer[0] != '\0' )
		{
			vCritical() << "macvnc_start() failed:" << errorBuffer;
		}
		else
		{
			vCritical() << "macvnc_start() failed";
		}

		return false;
	}

	vDebug() << "BuiltinMacVncServer::runServer(): macvnc_start() returned true, server should be running";

	bool interrupted = false;
	while( macvnc_is_running() )
	{
		if( QThread::currentThread()->isInterruptionRequested() )
		{
			interrupted = true;
			vDebug() << "macVNC interruption requested";
			macvnc_stop();
			break;
		}

		QThread::msleep( 250 );
	}

	if( macvnc_is_running() )
	{
		macvnc_stop();
	}

	char lastError[512] = {0};
	macvnc_get_last_error( lastError, sizeof(lastError) );
	if( interrupted == false && lastError[0] != '\0' )
	{
		vCritical() << "MacVNC server terminated unexpectedly:" << lastError;
		return false;
	}

	return true;
}


IMPLEMENT_CONFIG_PROXY(MacVncConfiguration)
