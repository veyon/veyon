/*
 * SoundMuteFeaturePlugin.cpp - implementation of SoundMuteFeaturePlugin class
 *
 * Copyright (c) 2017-2026 Tobias Junghans <tobydox@veyon.io>
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
#include <QStandardPaths>
#include <QDir>
#include <QProcess>
#include <QThread>

#include "SoundMuteFeaturePlugin.h"
#include "FeatureWorkerManager.h"
#include "PlatformCoreFunctions.h"
#include "PlatformSessionFunctions.h"
#include "VeyonServerInterface.h"


SoundMuteFeaturePlugin::SoundMuteFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_soundMuteFeature( QStringLiteral( "SoundMute" ),
						 Feature::Flag::Mode | Feature::Flag::AllComponents,
						 Feature::Uid( "ccb535a2-1d24-4cc2-a709-8b47d2b2ac79" ),
						 Feature::Uid(),
						 tr( "Mute" ), tr( "Unmute" ),
						 tr( "To prevent users disturbing your instructions by "
							 "playing sounds from their computers you can use "
							 "this button."
							 "In this mode all sound devices are muted." ),
						 QStringLiteral(":/soundmute/sound-mute.png") ),
	m_features( { m_soundMuteFeature } ),
	m_muteWidget( nullptr )
{
}



SoundMuteFeaturePlugin::~SoundMuteFeaturePlugin()
{
	delete m_muteWidget;
}



bool SoundMuteFeaturePlugin::controlFeature( Feature::Uid featureUid, Operation operation,
											 const QVariantMap& arguments,
											 const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(arguments)

	if( hasFeature( featureUid ) == false )
	{
		return false;
	}

	if( operation == Operation::Start )
	{
		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::StartMute}, computerControlInterfaces);

		return true;
	}

	if( operation == Operation::Stop )
	{
		sendFeatureMessage(FeatureMessage{featureUid, FeatureCommand::StopMute}, computerControlInterfaces);

		return true;
	}

	return false;
}



bool SoundMuteFeaturePlugin::handleFeatureMessage( VeyonServerInterface& server,
													const MessageContext& messageContext,
													const FeatureMessage& message )
{
	Q_UNUSED(messageContext)

	if( message.featureUid() == m_soundMuteFeature.uid() )
	{
		if (server.featureWorkerManager().isWorkerRunning( message.featureUid() ) == false &&
			message.command<FeatureCommand>() == FeatureCommand::StopMute)
		{
			return true;
		}

		if( VeyonCore::platform().sessionFunctions().currentSessionHasUser() == false )
		{
			vDebug() << "not muting sound since not running in a user session";
			return true;
		}

		// forward message to worker
		server.featureWorkerManager().sendMessageToManagedSystemWorker( message );

		return true;
	}

	return false;
}



bool SoundMuteFeaturePlugin::handleFeatureMessage( VeyonWorkerInterface& worker, const FeatureMessage& message )
{
	Q_UNUSED(worker);
	auto cmd = [](const char* s) { return QString::fromLatin1(s); };
	
	auto runCommand = [&](const QString &name, const QStringList &args) {
		QString fullPath = QStandardPaths::findExecutable(name);
		if (fullPath.isEmpty()) {
			if (QFile::exists(QStringLiteral("/usr/bin/") + name)) {
				fullPath = QStringLiteral("/usr/bin/") + name;
			} else if (QFile::exists(QStringLiteral("/bin/") + name)) {
				fullPath = QStringLiteral("/bin/") + name;
			}
		}

		if (!fullPath.isEmpty()) {
			return QProcess::execute(fullPath, args);
		}
	return -1; 
	};

	#ifdef Q_OS_LINUX
	QDir sndDir(QStringLiteral("/dev/snd"));
	QStringList controls = sndDir.entryList({QStringLiteral("controlC*")}, QDir::System);

	QStringList cardIndices;
	for(const QString &ctrl : controls) {
		QString index = ctrl;
		index.remove(QStringLiteral("controlC"));
		if(!index.isEmpty()) { cardIndices << index; }
	}
	#endif

	if( message.featureUid() == m_soundMuteFeature.uid() )
	{
		switch (message.command<FeatureCommand>())
		{
		case FeatureCommand::StartMute:
			if( m_muteWidget == nullptr )
			{
				m_muteWidget = new QWidget(nullptr);
				#ifdef Q_OS_LINUX
				runCommand(cmd("alsactl"), {cmd("--file"), m_audioState, cmd("store")});
				for(const QString &idx : cardIndices) {
					runCommand(cmd("amixer"), {cmd("-q"), cmd("-c"), idx, cmd("set"), cmd("Master"), cmd("mute")});
				}
				for (const QString &device : controls) {
					QString fullPath = QStringLiteral("/dev/snd/") + device;
					runCommand(cmd("chmod"), {cmd("444"), fullPath});
				}
				#endif
				#ifdef Q_OS_WIN
				QProcess::execute("net", QStringList() << "stop" << "audiosrv" << "/y");
				#endif
			}
			return true;

		case FeatureCommand::StopMute:
			delete m_muteWidget;
			m_muteWidget = nullptr;
			#ifdef Q_OS_LINUX
			for (const QString &device : controls) {
				QString fullPath = QStringLiteral("/dev/snd/") + device;
				runCommand(QStringLiteral("chmod"), {QStringLiteral("664"), fullPath});
			}
			QThread::msleep(500); 
			runCommand(cmd("alsactl"), {cmd("--file"), m_audioState, cmd("restore")});
			QFile::remove(m_audioState);
			#endif
			#ifdef Q_OS_WIN
			QProcess::execute("net", QStringList() << "start" << "audiosrv");
			QProcess::execute("net", QStringList() << "start" << "AudioEndpointBuilder");
			#endif

			QCoreApplication::quit();

			return true;

		default:
			break;
		}
	}

	return false;
}
