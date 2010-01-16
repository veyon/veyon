/*
 * dialogs.h - declaration of dialog-classes
 *
 * Copyright (c) 2006-2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QList>

#include "dialogs/wizard.uic"
#include "dialogs/page_welcome.uic"
#include "dialogs/page_security_options.uic"
#include "dialogs/page_key_dirs.uic"
#include "dialogs/page_setup_finished.uic"


class setupWizardPage;


class setupWizard : public QDialog, private Ui::wizard
{
	Q_OBJECT
public:
	setupWizard( const QString & installDir );

	QString m_installDir;
	QString m_keyImportDir;
	QString m_keyExportDir;
	QString m_pubKeyDir;
	QString m_privKeyDir;

	bool m_saveInstallSettings;

	void setNextPageDisabled( bool );

	void loadSettings( const QString & _install_settings );
	void doInstallation( bool _quiet = FALSE);

	int askOverwrite( const QString & _file, bool _all = FALSE );


private slots:
	virtual void reject();
	void back();
	void next();


private:
	virtual void closeEvent( QCloseEvent * _ev );

	QList<setupWizardPage *> m_widgetStack;
	int m_idx;
	bool m_closeOk;


	friend class setupWizardPage;

} ;



class setupWizardPage : public QWidget
{
public:
	setupWizardPage( setupWizard * _wiz );

	virtual bool nextPage()
	{
		return( !nextPageDisabled() );
	}

	virtual bool nextPageDisabled()
	{
		return( FALSE );
	}


protected:
	setupWizard * m_setupWizard;

} ;




class setupWizardPageWelcome : public setupWizardPage, public Ui::pageWelcome
{
public:
	setupWizardPageWelcome( setupWizard * _wiz );

} ;




class setupWizardPageSecurityOptions : public setupWizardPage,
						public Ui::pageSecurityOptions
{
	Q_OBJECT
public:
	setupWizardPageSecurityOptions( setupWizard * _wiz );

	virtual bool nextPageDisabled();


private slots:
	void openKeyImportDir();
	void setKeyImportDir( const QString & _dir );
	void createKeysRadioButtonToggled( bool );
	void keepKeysRadioButtonToggled( bool );


} ;




class setupWizardPageKeyDirs : public setupWizardPage, public Ui::pageKeyDirs
{
	Q_OBJECT
public:
	setupWizardPageKeyDirs( setupWizard * _wiz );

	virtual bool nextPageDisabled();


private slots:
	void openPubKeyDir();
	void openPrivKeyDir();
	void openKeyExportDir();
	void setPubKeyDir( const QString & _dir );
	void setPrivKeyDir( const QString & _dir );
	void setKeyExportDir( const QString & _dir );

} ;



class setupWizardPageFinished : public setupWizardPage, public Ui::pageFinished
{
	Q_OBJECT
public:
	setupWizardPageFinished( setupWizard * _wiz );

private slots:
	void toggleSaveInstallSettings( bool );

} ;





#endif
