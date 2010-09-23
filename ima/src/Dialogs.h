/*
 * Dialogs.h - declaration of dialog-classes
 *
 * Copyright (c) 2004-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#ifndef _DIALOGS_H
#define _DIALOGS_H

#include "ui_About.h"
#include "ui_ClientSettings.h"
#include "ui_TextMessage.h"
#include "ui_Support.h"
#include "ui_RemoteLogon.h"


class Client;
#ifndef ITALC3
class MainWindow;
#endif


class AboutDialog : public QDialog, private Ui::About
{
public:
	AboutDialog( QWidget * _parent );

} ;



class ClientSettingsDialog : public QDialog, private Ui::ClientSettings
{
	Q_OBJECT
public:
	ClientSettingsDialog( Client * _c,
#ifndef ITALC3
				MainWindow * _main_window,
#endif
				const QString & _classroom );

private:
	virtual void accept( void );

	Client * m_client;
#ifndef ITALC3
	MainWindow * m_mainWindow;
#endif

} ;




class SupportDialog : public QDialog, private Ui::Support
{
public:
	SupportDialog( QWidget * _parent ) :
		QDialog( _parent ? _parent->window() : 0 )
	{
		setupUi( this );
	}

	static QString getHost( QWidget * _parent )
	{
		SupportDialog sd( _parent );
		if( sd.exec() == Accepted )
		
		{
			return sd.hostEdit->text();
		}
		return QString();
	}

} ;




class TextMessageDialog : public QDialog, private Ui::TextMessage
{
	Q_OBJECT
public:
	TextMessageDialog( QString & _msg_str, QWidget * _parent = 0 );


private slots:
	virtual void accept( void );


private:
	QString & m_msgStr;

} ;




class RemoteLogonDialog : public QDialog, private Ui::RemoteLogon
{
	Q_OBJECT
public:
	RemoteLogonDialog( QWidget * _parent = 0 );

	const QString & userName( void ) const
	{
		return m_userName;
	}

	const QString & password( void ) const
	{
		return m_password;
	}

	const QString & domain( void ) const
	{
		return m_domain;
	}


private slots:
	void userNameChanged( const QString & _un )
	{
		m_userName = _un;
	}

	void passwordChanged( const QString & _pw )
	{
		m_password = _pw;
	}

	void domainChanged( const QString & _domain )
	{
		m_domain = _domain;
	}


private:
	QString m_userName;
	QString m_password;
	QString m_domain;

} ;


#endif
