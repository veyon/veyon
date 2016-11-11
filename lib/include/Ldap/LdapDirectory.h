/*
 * LdapDirectory.h - class representing the LDAP directory and providing access to directory entries
 *
 * Copyright (c) 2016 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 *
 * This file is part of iTALC - http://italc.sourceforge.net
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

class LdapDirectory : public QObject
{
	Q_OBJECT
public:
	LdapDirectory();
	virtual ~LdapDirectory();

	bool isConnected() const;
	bool isBound() const;

	void disableFilters();

	QString ldapErrorDescription() const;

	QStringList queryEntries( const QString& dn, const QString& attribute, const QString& filter );

	QStringList queryBaseDn( const QString& attribute );

	QString queryNamingContext();

	QStringList users( const QString& filterValue = QString() );
	QStringList groups( const QString& filterValue = QString() );
	QStringList computers( const QString& filter = QString() );
	QStringList computerPools( const QString& filterValue = QString() );

	QStringList groupMembers( const QString& groupName );

private:
	bool reconnect();

	static QString constructQueryFilter( const QString& filterAttribute,
										 const QString& filterValue,
										 const QString& extraFilter );

	class LdapDirectoryPrivate;

	QScopedPointer<LdapDirectoryPrivate> d;

};

#endif
