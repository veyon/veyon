/*
 * dialogs.h - declaration of dialog-classes
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include "dialogs/about.uic"
#include "dialogs/client_settings.uic"
#include "dialogs/text_message.uic"
#include "dialogs/support.uic"
#include "dialogs/multi_logon.uic"


class client;
class mainWindow;



class aboutDialog : public QDialog, private Ui::about
{
public:
	aboutDialog( QWidget * _parent );

} ;



class clientSettingsDialog : public QDialog, private Ui::clientSettings
{
	Q_OBJECT
public:
	clientSettingsDialog( client * _c, mainWindow * _main_window,
						const QString & _classroom );

private:
	virtual void accept( void );

	client * m_client;
	mainWindow * m_mainWindow;

} ;




class supportDialog : public QDialog, private Ui::support
{
public:
	supportDialog( QWidget * _parent ) :
		QDialog( _parent ? _parent->window() : 0 )
	{
		setupUi( this );
	}

	static QString getHost( QWidget * _parent )
	{
		supportDialog sd( _parent );
		if( sd.exec() == Accepted )
		
		{
			return( sd.hostEdit->text() );
		}
		return( "" );
	}

} ;




class textMessageDialog : public QDialog, private Ui::textMessage
{
	Q_OBJECT
public:
	textMessageDialog( QString & _msg_str, QWidget * _parent );


private slots:
	virtual void accept( void );


private:
	QString & m_msgStr;

} ;




class multiLogonDialog : public QDialog, private Ui::multiLogon
{
	Q_OBJECT
public:
	multiLogonDialog( QWidget * _parent );

	const QString & userName( void ) const
	{
		return( m_userName );
	}

	const QString & password( void ) const
	{
		return( m_password );
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


private:
	QString m_userName;
	QString m_password;

} ;


#endif
