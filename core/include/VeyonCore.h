/*
 * VeyonCore.h - definitions for veyon Core
 *
 * Copyright (c) 2006-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

template<class A, class B>
static inline bool intersects( const QSet<A>& a, const QSet<B>& b )
{
#if QT_VERSION < 0x050600
	return QSet<A>( a ).intersect( b ).isEmpty() == false;
#else
	return a.intersects( b );
#endif
}

#if QT_VERSION < 0x050700
template <typename T> struct QAddConst { typedef const T Type; };
template <typename T> constexpr typename QAddConst<T>::Type &qAsConst(T &t) { return t; }
template <typename T> void qAsConst(const T &&) = delete;

template <typename... Args>
struct QNonConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};

template <typename... Args>
struct QConstOverload
{
	template <typename R, typename T>
	Q_DECL_CONSTEXPR auto operator()(R (T::*ptr)(Args...) const) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R, typename T>
	static Q_DECL_CONSTEXPR auto of(R (T::*ptr)(Args...) const) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
	using QConstOverload<Args...>::of;
	using QConstOverload<Args...>::operator();
	using QNonConstOverload<Args...>::of;
	using QNonConstOverload<Args...>::operator();

	template <typename R>
	Q_DECL_CONSTEXPR auto operator()(R (*ptr)(Args...)) const Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }

	template <typename R>
	static Q_DECL_CONSTEXPR auto of(R (*ptr)(Args...)) Q_DECL_NOTHROW -> decltype(ptr)
	{ return ptr; }
};
#endif

class QCoreApplication;
class QWidget;

class AccessControlDataBackendManager;
class AuthenticationCredentials;
class CryptoCore;
class VeyonConfiguration;
class Logger;
class PlatformPluginInterface;
class PlatformPluginManager;
class PluginManager;

class VEYON_CORE_EXPORT VeyonCore : public QObject
{
	Q_OBJECT
public:
	VeyonCore( QCoreApplication* application, const QString& appComponentName, QObject* parent = nullptr );
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

	static PlatformPluginInterface& platform()
	{
		return *( instance()->m_platformPlugin );
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
	PlatformPluginManager* m_platformPluginManager;
	PlatformPluginInterface* m_platformPlugin;

	QString m_applicationName;
	UserRole m_userRole;

};

#endif
