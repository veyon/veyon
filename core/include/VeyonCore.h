/*
 * VeyonCore.h - definitions for veyon Core
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of veyon - http://veyon.io
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

#ifndef VEYON_CORE_H
#define VEYON_CORE_H

#include <veyonconfig.h>

#ifdef VEYON_BUILD_WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#include "rfb/rfbproto.h"

#include <QtEndian>
#include <QString>
#include <QDebug>

#if defined(BUILD_VEYON_CORE_LIBRARY)
#  define VEYON_CORE_EXPORT Q_DECL_EXPORT
#else
#  define VEYON_CORE_EXPORT Q_DECL_IMPORT
#endif

class QCoreApplication;
class QWidget;

class AccessControlDataBackendManager;
class AuthenticationCredentials;
class CryptoCore;
class VeyonConfiguration;
class Logger;
class PluginManager;

class VEYON_CORE_EXPORT VeyonCore : public QObject
{
	Q_OBJECT
public:
	VeyonCore( QCoreApplication* application, const QString& appComponentName );
	~VeyonCore() override;

	static VeyonCore* instance();

	static VeyonConfiguration& config()
	{
		return *( instance()->m_config );
	}

	static AuthenticationCredentials& authenticationCredentials()
	{
		return *( instance()->m_authenticationCredentials );
	}

	static PluginManager& pluginManager()
	{
		return *( instance()->m_pluginManager );
	}

	static AccessControlDataBackendManager& accessControlDataBackendManager()
	{
		return *( instance()->m_accessControlDataBackendManager );
	}

	static void setupApplicationParameters();
	bool initAuthentication( int credentialTypes );

	static QString applicationName();
	static void enforceBranding( QWidget* topLevelWidget );

	typedef enum UserRoles
	{
		RoleNone,
		RoleTeacher,
		RoleAdmin,
		RoleSupporter,
		RoleOther,
		RoleCount
	} UserRole;

	Q_ENUM(UserRole)

	static QString userRoleName( UserRole role );

	UserRole userRole()
	{
		return m_userRole;
	}

	void setUserRole( UserRole userRole )
	{
		m_userRole = userRole;
	}

private:
	static VeyonCore* s_instance;

	VeyonConfiguration* m_config;
	Logger* m_logger;
	AuthenticationCredentials* m_authenticationCredentials;
	CryptoCore* m_cryptoCore;
	PluginManager* m_pluginManager;
	AccessControlDataBackendManager* m_accessControlDataBackendManager;

	QString m_applicationName;
	UserRole m_userRole;

};

#endif
