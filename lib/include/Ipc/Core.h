/*
 * IpcCore.h - core definitions for the IPC framework
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
 * Copyright (c) 2010 Univention GmbH
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

#ifndef _IPC_CORE_H
#define _IPC_CORE_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Ipc
{
	typedef QString Command;
	typedef QString Id;
	typedef QString Argument;
	typedef QMap<Argument, QVariant> CommandArgs;

	namespace Commands
	{
		extern const Command Identify;
		extern const Command UnknownCommand;
		extern const Command Ping;
		extern const Command Quit;
	}

	namespace Arguments
	{
		extern const Argument Id;
		extern const Argument Command;
	}

	class Msg
	{
	public:
		Msg( const Command &cmd = Command() ) :
			m_cmd( cmd )
		{
		}

		Msg( const Msg & msg ) :
			m_cmd( msg.m_cmd ),
			m_args( msg.m_args )
		{
		}

		bool isValid() const
		{
			return !cmd().isEmpty();
		}

		const Command & cmd() const
		{
			return m_cmd;
		}

		const CommandArgs & args() const
		{
			return m_args;
		}

		Msg & addArg( const QString &key, const QVariant &value )
		{
			m_args[key] = value;
			return *this;
		}

		Msg & addArg( const QString &key, const int value )
		{
			m_args[key] = QString::number( value );
			return *this;
		}

		QString arg( const QString &key ) const
		{
			return m_args[key].toString();
		}

		QVariant argV( const QString &key ) const
		{
			return m_args[key];
		}

		template<class QIOD>
		bool send( QIOD *d ) const
		{
			QDataStream ds( d );
			ds << m_cmd;
			ds << m_args;
			d->flush();
			return true;
		}

		template<class QIOD>
		Msg & receive( QIOD *d )
		{
			QDataStream ds( d );
			ds >> m_cmd;
			ds >> m_args;
			return *this;
		}


	private:
		Command m_cmd;
		CommandArgs m_args;

	} ;

}

#endif // _IPC_CORE_H
