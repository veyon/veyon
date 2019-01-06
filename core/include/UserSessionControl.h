/*
 * UserSessionControl.h - declaration of UserSessionControl class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - http://veyon.io
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

#ifndef USER_SESSION_CONTROL_H
#define USER_SESSION_CONTROL_H

#include <QReadWriteLock>

#include "SimpleFeatureProvider.h"

class QThread;
class QTimer;

class VEYON_CORE_EXPORT UserSessionControl : public QObject, public SimpleFeatureProvider, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(FeatureProviderInterface PluginInterface)
public:
	UserSessionControl( QObject* parent = nullptr );
	~UserSessionControl() override;

	bool getUserSessionInfo( const ComputerControlInterfaceList& computerControlInterfaces );

	Plugin::Uid uid() const override
	{
		return QStringLiteral("80580500-2e59-4297-9e35-e53959b028cd");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 1 );
	}

	QString name() const override
	{
		return QStringLiteral( "UserSessionControl" );
	}

	QString description() const override
	{
		return tr( "User session control" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	const FeatureList& featureList() const override
	{
		return m_features;
	}

	bool startFeature( VeyonMasterInterface& master, const Feature& feature,
					   const ComputerControlInterfaceList& computerControlInterfaces ) override;

	bool handleFeatureMessage( VeyonMasterInterface& master, const FeatureMessage& message,
							   ComputerControlInterface::Pointer computerControlInterface ) override;

	bool handleFeatureMessage( VeyonServerInterface& server, const FeatureMessage& message ) override;

private:
	enum Commands
	{
		GetInfo,
		LogonUser,
		LogoutUser
	};

	enum Arguments
	{
		UserName,
	};

	void queryUserInformation();
	bool confirmFeatureExecution( const Feature& feature, QWidget* parent );

	const Feature m_userSessionInfoFeature;
	const Feature m_userLogoutFeature;
	const FeatureList m_features;

	QThread* m_userInfoQueryThread;
	QTimer* m_userInfoQueryTimer;

	QReadWriteLock m_userDataLock;
	QString m_userName;
	QString m_userFullName;

};

#endif // USER_SESSION_CONTROL_H
