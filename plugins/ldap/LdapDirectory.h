/*
 * LdapDirectory.h - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016-2017 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef LDAP_DIRECTORY_H
#define LDAP_DIRECTORY_H

#include <QObject>
#include <QUrl>

#include "VeyonCore.h"

class LdapConfiguration;

class LdapDirectory : public QObject
{
	Q_OBJECT
public:
	LdapDirectory( const LdapConfiguration& configuration, const QUrl& url = QUrl() );
	~LdapDirectory() override;

	bool isConnected() const;
	bool isBound() const;

	void disableAttributes();
	void disableFilters();

	QString ldapErrorDescription() const;

	QStringList queryBaseDn();

	QString queryNamingContext();

	QString toRelativeDn( QString fullDn );
	QString toFullDn( QString relativeDn );

	QStringList toRelativeDnList( const QStringList& fullDnList );

	QStringList users( const QString& filterValue = QString() );
	QStringList groups( const QString& filterValue = QString() );
	QStringList userGroups( const QString& filterValue = QString() );
	QStringList computers( const QString& filterValue = QString() );
	QStringList computerGroups( const QString& filterValue = QString() );
	QStringList computerLabs( const QString& filterValue = QString() );

	QStringList groupMembers( const QString& groupDn );
	QStringList groupsOfUser( const QString& userDn );
	QStringList groupsOfComputer( const QString& computerDn );
	QStringList computerLabsOfComputer( const QString& computerDn );
	QStringList commonAggregations( const QString& objectOne, const QString& objectTwo );

	QString userLoginName( const QString& userDn );
	QString groupName( const QString& groupDn );
	QString computerHostName( const QString& computerDn );
	QString computerMacAddress( const QString& computerDn );
	QString groupMemberUserIdentification( const QString& userDn );
	QString groupMemberComputerIdentification( const QString& computerDn );

	QStringList computerLabMembers( const QString& computerLabName );

	QString hostToLdapFormat( const QString& host );
	QString computerObjectFromHost( const QString& host );


private:
	bool reconnect( const QUrl& url );

	static QString constructQueryFilter( const QString& filterAttribute,
										 const QString& filterValue,
										 const QString& extraFilter = QString() );

	class LdapDirectoryPrivate;

	const LdapConfiguration& m_configuration;
	QScopedPointer<LdapDirectoryPrivate> d;

};

#endif
