/*
 * RemoteAccessFeaturePlugin.cpp - implementation of RemoteAccessFeaturePlugin class
 *
 * Copyright (c) 2017-2025 Tobias Junghans <tobydox@veyon.io>
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

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QInputDialog>

#include "AuthenticationManager.h"
#include "FeatureWorkerManager.h"
#include "RemoteAccessFeaturePlugin.h"
#include "RemoteAccessPage.h"
#include "RemoteAccessWidget.h"
#include "VeyonConfiguration.h"
#include "VeyonMasterInterface.h"
#include "VeyonServerInterface.h"
#include "VncView.h"


RemoteAccessFeaturePlugin::RemoteAccessFeaturePlugin( QObject* parent ) :
	QObject( parent ),
	m_remoteViewFeature( QStringLiteral( "RemoteView" ),
						 Feature::Flag::Session | Feature::Flag::Master,
						 Feature::Uid( "a18e545b-1321-4d4e-ac34-adc421c6e9c8" ),
						 Feature::Uid(),
						 tr( "Remote view" ), {},
						 tr( "Open a remote view for a computer without interaction." ),
						 QStringLiteral(":/remoteaccess/kmag.png") ),
	m_remoteControlFeature( QStringLiteral( "RemoteControl" ),
							Feature::Flag::Session | Feature::Flag::Master,
							Feature::Uid( "ca00ad68-1709-4abe-85e2-48dff6ccf8a2" ),
							Feature::Uid(),
							tr( "Remote control" ), {},
							tr( "Open a remote control window for a computer." ),
							QStringLiteral(":/remoteaccess/krdc.png") ),
	m_clipboardExchangeFeature(QStringLiteral("ClipboardExchange"),
								Feature::Flag::Meta,
								Feature::Uid("8fa73e19-3d66-4d59-9783-c2a1bb07e20e"),
								Feature::Uid(),
								tr("Exchange clipboard contents"), {}, {}),
	m_features({m_remoteViewFeature, m_remoteControlFeature, m_clipboardExchangeFeature}),
	m_commands( {
{ QStringLiteral("view"), m_remoteViewFeature.displayName() },
{ QStringLiteral("control"), m_remoteControlFeature.displayName() },
{ QStringLiteral("help"), tr( "Show help about command" ) },
				} ),
	m_clipboardSynchronizationDisabled(VeyonCore::config().clipboardSynchronizationDisabled())
{
	if (VeyonCore::component() == VeyonCore::Component::Server && m_clipboardSynchronizationDisabled == false)
	{
		connect (QGuiApplication::clipboard(), &QClipboard::dataChanged,
				 this, &RemoteAccessFeaturePlugin::updateClipboardData);
	}
}



const FeatureList &RemoteAccessFeaturePlugin::featureList() const
{
	return m_features;
}



bool RemoteAccessFeaturePlugin::controlFeature(Feature::Uid featureUid, Operation operation, const QVariantMap& arguments,
											   const ComputerControlInterfaceList& computerControlInterfaces)
{
	if (hasFeature(featureUid) &&
		operation == Operation::Start)
	{
		sendFeatureMessage(FeatureMessage{featureUid}
								.addArgument(Argument::HostName, arguments.value(argToString(Argument::HostName))),
							computerControlInterfaces);
		return true;
	}

	return false;
}



bool RemoteAccessFeaturePlugin::startFeature( VeyonMasterInterface& master, const Feature& feature,
											  const ComputerControlInterfaceList& computerControlInterfaces )
{
	Q_UNUSED(computerControlInterfaces)

	if( hasFeature( feature.uid() ) == false )
	{
		return false;
	}

	const auto viewOnly = feature.uid() == m_remoteViewFeature.uid() ||
						  remoteControlEnabled() == false;

	const auto selectedComputerControlInterfaces = master.selectedComputerControlInterfaces();

	if (selectedComputerControlInterfaces.count() > 0)
	{
		for (const auto& computerControlInterface : selectedComputerControlInterfaces)
		{
			createRemoteAccessWindow(computerControlInterface, viewOnly, &master);
		}
	}
	else
	{
		const auto hostName = QInputDialog::getText( master.mainWindow(),
													 tr( "Remote access" ),
													 tr( "No computer has been selected so you can enter a hostname "
														 "or IP address of a computer for manual access:" ) );
		if( hostName.isEmpty() )
		{
			return false;
		}

		Computer customComputer;
		customComputer.setHostAddress( hostName );
		customComputer.setName( hostName );

		createRemoteAccessWindow(ComputerControlInterface::Pointer::create(customComputer), viewOnly, &master);
	}

	return true;
}



bool RemoteAccessFeaturePlugin::handleFeatureMessage(ComputerControlInterface::Pointer computerControlInterface,
													 const FeatureMessage& message)
{
	Q_UNUSED(computerControlInterface)

	if (message.featureUid() == m_clipboardExchangeFeature.uid() && m_clipboardSynchronizationDisabled == false)
	{
		for (auto it = m_vncViews.constBegin(), end = m_vncViews.constEnd(); it != end; ++it)
		{
			if (it->first && it->second->computerControlInterface() == computerControlInterface)
			{
				loadClipboardData(message);
			}
		}

		return true;
	}

	return false;
}



bool RemoteAccessFeaturePlugin::handleFeatureMessage(VeyonServerInterface &server,
													 const MessageContext &messageContext,
													 const FeatureMessage &message)
{
	Q_UNUSED(messageContext)

	if (message.featureUid() == m_remoteViewFeature.uid() ||
		message.featureUid() == m_remoteControlFeature.uid())
	{
		// forward message to worker
		server.featureWorkerManager().sendMessageToUnmanagedSessionWorker(message);
		return true;
	}
	else if (message.featureUid() == m_clipboardExchangeFeature.uid() && m_clipboardSynchronizationDisabled == false)
	{
		loadClipboardData(message);
		return true;
	}

	return false;
}



bool RemoteAccessFeaturePlugin::handleFeatureMessage(VeyonWorkerInterface &worker, const FeatureMessage &message)
{
	Q_UNUSED(worker)

	if (message.featureUid() == m_remoteViewFeature.uid() ||
		message.featureUid() == m_remoteControlFeature.uid())
	{
		const auto viewOnly = message.featureUid() == m_remoteViewFeature.uid() ||
							  remoteControlEnabled() == false;

		remoteAccess(message.argument(Argument::HostName).toString(), viewOnly);

		return true;
	}

	return false;
}



void RemoteAccessFeaturePlugin::sendAsyncFeatureMessages(VeyonServerInterface& server,
														 const MessageContext& messageContext)
{
	const auto clipboardDataVersion = messageContext.ioDevice()->property(clipboardDataVersionProperty()).toInt();

	if (m_clipboardSynchronizationDisabled == false && clipboardDataVersion != m_clipboardDataVersion)
	{
		FeatureMessage message{m_clipboardExchangeFeature.uid()};

		m_clipboardDataMutex.lock();
		storeClipboardData(&message, m_clipboardText, m_clipboardImage);
		m_clipboardDataMutex.unlock();

		server.sendFeatureMessageReply(messageContext, message);
		messageContext.ioDevice()->setProperty(clipboardDataVersionProperty(), m_clipboardDataVersion);
	}
}



QStringList RemoteAccessFeaturePlugin::commands() const
{
	return m_commands.keys();
}



QString RemoteAccessFeaturePlugin::commandHelp( const QString& command ) const
{
	return m_commands.value( command );
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_view( const QStringList& arguments )
{
	if( arguments.count() < 1 )
	{
		return NotEnoughArguments;
	}

	if( remoteViewEnabled() == false )
	{
		return InvalidCommand;
	}

	if (remoteAccess(arguments.first(), true))
	{
		qApp->exec();
		return Successful;
	}

	return Failed;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_control( const QStringList& arguments )
{
	if( arguments.count() < 1 )
	{
		return NotEnoughArguments;
	}

	if( remoteControlEnabled() == false )
	{
		return InvalidCommand;
	}

	if (remoteAccess(arguments.first(), false))
	{
		qApp->exec();
		return Successful;
	}

	return Failed;
}



CommandLinePluginInterface::RunResult RemoteAccessFeaturePlugin::handle_help( const QStringList& arguments )
{
	if( arguments.value( 0 ) == QLatin1String("view") )
	{
		printf( "\nremoteaccess view <host>\n\n" );
		return NoResult;
	}

	if( arguments.value( 0 ) == QLatin1String("control") )
	{
		printf( "\nremoteaccess control <host>\n}n" );
		return NoResult;
	}

	return InvalidCommand;
}



bool RemoteAccessFeaturePlugin::remoteViewEnabled() const
{
	return VeyonCore::config().disabledFeatures().contains( m_remoteViewFeature.uid().toString() ) == false;

}



bool RemoteAccessFeaturePlugin::remoteControlEnabled() const
{
	return VeyonCore::config().disabledFeatures().contains( m_remoteControlFeature.uid().toString() ) == false;
}



bool RemoteAccessFeaturePlugin::initAuthentication()
{
	return VeyonCore::authenticationManager().initializeCredentials() &&
			VeyonCore::authenticationManager().initializedPlugin()->checkCredentials();
}



bool RemoteAccessFeaturePlugin::remoteAccess( const QString& hostAddress, bool viewOnly )
{
	if( initAuthentication() == false )
	{
		return false;
	}

	Computer remoteComputer;
	remoteComputer.setName( hostAddress );
	remoteComputer.setHostAddress( hostAddress );

	if( remoteControlEnabled() == false )
	{
		viewOnly = true;
	}

	createRemoteAccessWindow(ComputerControlInterface::Pointer::create(remoteComputer), viewOnly, nullptr);

	return true;
}



void RemoteAccessFeaturePlugin::createRemoteAccessWindow(const ComputerControlInterface::Pointer& computerControlInterface,
														 bool viewOnly, VeyonMasterInterface* master)
{
	VncView* vncView{nullptr};
	QObject* remoteAccessView{nullptr};

	if (master && master->appContainer())
	{
		auto page = new RemoteAccessPage(computerControlInterface, viewOnly, master->appContainer());
		// forward clipboard changes as long as the page exists
		connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, page, [=]()
				 {
					 sendClipboardData(page->computerControlInterface());
				 });

		remoteAccessView = page;
		vncView = page->vncView();
	}
	else
	{
		auto widget = new RemoteAccessWidget(computerControlInterface, viewOnly,
											  remoteViewEnabled() && remoteControlEnabled());

		// forward clipboard changes as long as the widget exists
		connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, widget, [=]()
				 {
					 sendClipboardData(widget->computerControlInterface());
				 });

		remoteAccessView = widget;
		vncView = widget->vncView();
	}

	connect(remoteAccessView, &QObject::destroyed, this, [this](QObject* view) {
		for (auto it = m_vncViews.begin(); it != m_vncViews.end();) // clazy:exclude=detaching-member
		{
			if (it->first == nullptr || it->first == view)
			{
				it = m_vncViews.erase(it);
			}
			else
			{
				it++;
			}
		}
	});

	m_vncViews.append(qMakePair(remoteAccessView, vncView));
}



void RemoteAccessFeaturePlugin::storeClipboardData(FeatureMessage *message, const QString& text, const QImage& image)
{
	QBuffer buffer;
	buffer.open(QIODevice::WriteOnly);
	image.save(&buffer, clipboardImageFormat());
	buffer.close();

	message->addArgument(Argument::ClipboardText, text);
	message->addArgument(Argument::ClipboardImage, buffer.data());
}



void RemoteAccessFeaturePlugin::loadClipboardData(const FeatureMessage &message)
{
	if (m_clipboardSynchronizationDisabled)
	{
		return;
	}

	const auto clipboard = QGuiApplication::clipboard();

	const auto text = message.argument(Argument::ClipboardText).toString();
	if (text.isEmpty() == false && clipboard->text() != text)
	{
		clipboard->setText(text);
	}

	// TODO: better support for image I/O on Windows via QWindowsMime

	const auto image = QImage::fromData(message.argument(Argument::ClipboardImage).toByteArray(),
										 clipboardImageFormat());
	if (image.isNull() == false && clipboard->image() != image)
	{
		clipboard->setImage(image);
	}
}



void RemoteAccessFeaturePlugin::sendClipboardData(ComputerControlInterface::Pointer computerControlInterface)
{
	if (m_clipboardSynchronizationDisabled)
	{
		return;
	}

	FeatureMessage message{m_clipboardExchangeFeature.uid()};

	const auto clipboard = QGuiApplication::clipboard();

	// TODO: better support for image I/O on Windows via QWindowsMime
	storeClipboardData(&message, clipboard->text(), clipboard->image());

	computerControlInterface->sendFeatureMessage(message);
}



void RemoteAccessFeaturePlugin::updateClipboardData()
{
	if (m_clipboardSynchronizationDisabled)
	{
		return;
	}

	m_clipboardDataMutex.lock();

	const auto clipboard = QGuiApplication::clipboard();

	if (m_clipboardText != clipboard->text())
	{
		m_clipboardText = clipboard->text();
		++m_clipboardDataVersion;
	}

	// TODO: better support for image I/O on Windows via QWindowsMime
	if (m_clipboardImage != clipboard->image())
	{
		m_clipboardImage = clipboard->image();
		++m_clipboardDataVersion;
	}

	m_clipboardDataMutex.unlock();
}
