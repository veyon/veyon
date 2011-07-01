/*
 * Dialogs.h - declaration of dialog classes
 *
 * Copyright (c) 2004-2011 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtGui/QDialog>

namespace Ui
{
	class ClientSettingsDialog;
	class TextMessageDialog;
	class SupportDialog;
	class RemoteLogonDialog;
}

class Client;
#ifndef ITALC3
class MainWindow;
#endif


class ClientSettingsDialog : public QDialog
{
	Q_OBJECT
public:
	ClientSettingsDialog( Client *c,
#ifndef ITALC3
				MainWindow * _main_window,
#endif
				const QString &classroom );

private:
	virtual void accept();

	Ui::ClientSettingsDialog *ui;
	Client *m_client;
#ifndef ITALC3
	MainWindow * m_mainWindow;
#endif

} ;




class SupportDialog : public QDialog
{
public:
	SupportDialog( QWidget *parent );

	static QString getHost( QWidget *parent );


private:
	Ui::SupportDialog *ui;

} ;




class TextMessageDialog : public QDialog
{
	Q_OBJECT
public:
	TextMessageDialog( QString &msgStr, QWidget *parent );


private slots:
	virtual void accept();


private:
	Ui::TextMessageDialog *ui;
	QString &m_msgStr;

} ;




class RemoteLogonDialog : public QDialog
{
	Q_OBJECT
public:
	RemoteLogonDialog( QWidget *parent );

	const QString &userName() const
	{
		return m_userName;
	}

	const QString &password() const
	{
		return m_password;
	}

	const QString &domain() const
	{
		return m_domain;
	}


private slots:
	void userNameChanged( const QString &un )
	{
		m_userName = un;
	}

	void passwordChanged( const QString &pw )
	{
		m_password = pw;
	}

	void domainChanged( const QString &domain )
	{
		m_domain = domain;
	}


private:
	Ui::RemoteLogonDialog *ui;
	QString m_userName;
	QString m_password;
	QString m_domain;

} ;


#endif
