/*
 * dialogs.h - declaration of dialog-classes
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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
#include "dialogs/page_license_agreement.uic"
#include "dialogs/page_install_dir.uic"
#include "dialogs/page_select_components.uic"
#include "dialogs/page_security_options.uic"
#include "dialogs/page_key_dirs.uic"


class setupWizardPage;


class setupWizard : public QDialog, private Ui::wizard
{
	Q_OBJECT
public:
	setupWizard();

	QString m_installDir;
	QString m_keyImportDir;
	QString m_keyExportDir;
	QString m_pubKeyDir;
	QString m_privKeyDir;

	bool m_installClient;
	bool m_installMaster;
	bool m_installLUPUS;
	bool m_installDocs;

	void setNextPageDisabled( bool );


private slots:
	virtual void reject( void );
	void back( void );
	void next( void );


private:
	QList<setupWizardPage *> m_widgetStack;
	int m_idx;


	friend class setupWizardPage;

} ;



class setupWizardPage : public QWidget
{
public:
	setupWizardPage( setupWizard * _wiz );

	virtual bool nextPage( void )
	{
		return( !nextPageDisabled() );
	}

	virtual bool nextPageDisabled( void )
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




class setupWizardPageLicenseAgreement : public setupWizardPage,
						public Ui::pageLicenseAgreement
{
	Q_OBJECT
public:
	setupWizardPageLicenseAgreement( setupWizard * _wiz );

	virtual bool nextPageDisabled( void )
	{
		return( !agreeRadioButton->isChecked() );
	}


private slots:
	void agreeRadioButtonToggled( bool );

} ;




class setupWizardPageInstallDir : public setupWizardPage,
						public Ui::pageInstallDir
{
	Q_OBJECT
public:
	setupWizardPageInstallDir( setupWizard * _wiz );

	virtual bool nextPage( void )
	{
		return( returnPressed() );
	}


private slots:
	bool returnPressed( void );
	void openDir( void );
	void setInstallDir( const QString & _dir );

} ;




class setupWizardPageSelectComponents : public setupWizardPage,
						public Ui::pageSelectComponents
{
	Q_OBJECT
public:
	setupWizardPageSelectComponents( setupWizard * _wiz );


private slots:
	void toggleComponentClient( bool );
	void toggleComponentMaster( bool );
	void toggleComponentLUPUS( bool );
	void toggleComponentDocs( bool );

} ;




class setupWizardPageSecurityOptions : public setupWizardPage,
						public Ui::pageSecurityOptions
{
	Q_OBJECT
public:
	setupWizardPageSecurityOptions( setupWizard * _wiz );

	virtual bool nextPageDisabled( void );


private slots:
	void openKeyImportDir( void );
	void setKeyImportDir( const QString & _dir );
	void createKeysRadioButtonToggled( bool );


} ;




class setupWizardPageKeyDirs : public setupWizardPage, public Ui::pageKeyDirs
{
	Q_OBJECT
public:
	setupWizardPageKeyDirs( setupWizard * _wiz );

	virtual bool nextPageDisabled( void );


private slots:
	void openPubKeyDir( void );
	void openPrivKeyDir( void );
	void openKeyExportDir( void );
	void setPubKeyDir( const QString & _dir );
	void setPrivKeyDir( const QString & _dir );
	void setKeyExportDir( const QString & _dir );

} ;




#endif
