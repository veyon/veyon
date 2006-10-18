/*
 * dialogs.cpp - implementation of dialogs
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QProcess>
#include <QtGui/QFileDialog>
#include <QtGui/QLayout>
#include <QtGui/QMessageBox>

#include "dialogs.h"
#include "local_system.h"
#include "dsa_key.h"
#include "qt_features.h"

#ifdef COMPLETER_SUPPORT
#include <QtGui/QDirModel>
#include <QtGui/QCompleter>
#endif


const QString PUBLIC_KEY_FILE_NAME = "italc_dsa_key.pub";
const QString DEFAULT_INSTALL_DIR =
#ifdef BUILD_WIN32
					"c:\\italc"
#else
					"/opt/italc"
#endif
					;

setupWizard::setupWizard() :
	QDialog(),
	Ui::wizard(),
	m_installDir( DEFAULT_INSTALL_DIR ),
	m_keyImportDir( QDir::homePath() + QDir::separator() ),
	m_keyExportDir( m_keyImportDir ),
	m_pubKeyDir( localSystem::publicKeyPath( ISD::RoleTeacher ) ),
	m_privKeyDir( localSystem::privateKeyPath( ISD::RoleTeacher ) ),
	m_widgetStack(),
	m_idx( 0 )
{
	setupUi( this );

	connect( backButton, SIGNAL( clicked() ), this, SLOT( back() ) );
	connect( nextButton, SIGNAL( clicked() ), this, SLOT( next() ) );

	nextButton->setFocus();


	QHBoxLayout * hboxLayout = new QHBoxLayout( contentWidget );
	hboxLayout->setMargin( 0 );
	hboxLayout->setSpacing( 0 );

	m_widgetStack << new setupWizardPageWelcome( this );
	m_widgetStack << new setupWizardPageLicenseAgreement( this );
	m_widgetStack << new setupWizardPageInstallDir( this );
	m_widgetStack << new setupWizardPageSelectComponents( this );
	m_widgetStack << new setupWizardPageSecurityOptions( this );
	m_widgetStack << new setupWizardPageKeyDirs( this );

	QSize max = m_widgetStack.front()->sizeHint();

	foreach( QWidget * w, m_widgetStack )
	{
		max = QSize( qMax( max.width(), w->sizeHint().width() ),
				qMax( max.height(), w->sizeHint().height() ) );
		w->hide();
	}
	contentWidget->setMinimumSize( max );
	m_widgetStack[m_idx]->show();
}




void setupWizard::setNextPageDisabled( bool _disabled )
{
	nextButton->setEnabled( !_disabled );
}




void setupWizard::back( void )
{
	if( m_idx > 0 )
	{
		m_widgetStack[m_idx]->hide();
		m_widgetStack[--m_idx]->show();
		setNextPageDisabled( m_widgetStack[m_idx]->nextPageDisabled() );
		nextButton->setText( tr( "Next" ) );
		nextButton->setFocus();
	}
}




void setupWizard::next( void )
{
	if( !m_widgetStack[m_idx]->nextPage() )
	{
		return;
	}
	if( m_idx+1 < m_widgetStack.size() )
	{
		m_widgetStack[m_idx]->hide();
		m_widgetStack[++m_idx]->show();
		if( m_widgetStack[m_idx]->nextPageDisabled() )
		{
			setNextPageDisabled( TRUE );
		}
		if( m_idx+1 == m_widgetStack.size() )
		{
			nextButton->setText( tr( "Finish" ) );
		}
		nextButton->setFocus();
	}
	else
	{
		const QString & d = m_installDir + QDir::separator();
		QStringList files;
		files <<
#ifdef BUILD_WIN32
			"ica.exe" 		<<
			"italc.exe" 		<<
			"libeay32.dll" 		<<
			"libjpeg.dll" 		<<
			"libssl32.dll" 		<<
			"libz.dll" 		<<
			"mingwm10.dll" 		<<
			"QtCore4.dll"		<<
			"QtGui4.dll"		<<
			"QtNetwork4.dll"	<<
			"QtXml4.dll"		<<
			"userinfo.exe"		<<
			"wake.exe"
#else
			"ica"			<<
			"italc"			<<
			"libssl.so.0.9.8"	<<
			"libcrypto.so.0.9.8"	<<
			"libz.so.1"		<<
			"libjpeg.so.62"		<<
			"libpng12.so.0"		<<
			"QtCore.so.4"		<<
			"QtGui.so.4"		<<
			"QtNetwork.so.4"	<<
			"QtXml.so.4"
#endif
			;
		bool overwrite_all = FALSE;
		bool overwrite_none = FALSE;
		foreach( const QString & file, files )
		{
			if( QFileInfo( d + file ).exists() && !overwrite_all )
			{
				if( overwrite_none )
				{
					continue;
				}
	int res = QMessageBox::question( NULL, tr( "Confirm overwrite" ),
			tr( "Do you want to overwrite %1?" ).arg( d + file ),
				QMessageBox::Yes | QMessageBox::No |
				QMessageBox::YesToAll | QMessageBox::NoToAll,
							QMessageBox::Yes );
				switch( res )
				{
					case QMessageBox::YesToAll:
						overwrite_all = TRUE;
						break;
					case QMessageBox::NoToAll:
						overwrite_none = TRUE;
					case QMessageBox::No:
						continue;
				}
				if( file == "ica" || file == "ica.exe" )
				{
					QProcess::execute( d + file +
							" -stopservice" );
					QProcess::execute( d + file +
							" -unregisterservice" );
				}
			}
			QFile( file ).copy( d + file );
		}
		// TODO: create/import/export keys
		QProcess::execute( d + "ica -registerservice" );
		// TODO: feedback
		accept();
	}
}






setupWizardPage::setupWizardPage( setupWizard * _wiz ) :
	QWidget( _wiz->contentWidget ),
	m_setupWizard( _wiz )
{
	parentWidget()->layout()->addWidget( this );
}




setupWizardPageWelcome::setupWizardPageWelcome( setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageWelcome()
{
	setupUi( this );
}






// setupWizardPageLicenseAgreement
setupWizardPageLicenseAgreement::setupWizardPageLicenseAgreement(
							setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageLicenseAgreement()
{
	setupUi( this );

        QFile copying_rc( ":/COPYING" );
	copying_rc.open( QFile::ReadOnly );
	licenseView->setHtml( QString( "<font size='3'><pre>%1</pre></font>" ).
				arg( QString( copying_rc.readAll() ) ) );

	connect( agreeRadioButton, SIGNAL( toggled( bool ) ),
			this, SLOT( agreeRadioButtonToggled( bool ) ) );
}




void setupWizardPageLicenseAgreement::agreeRadioButtonToggled( bool _on )
{
	m_setupWizard->setNextPageDisabled( !_on );
}






// setupWizardPageInstallDir
setupWizardPageInstallDir::setupWizardPageInstallDir( setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageInstallDir()
{
	setupUi( this );

	installDirLineEdit->setText( m_setupWizard->m_installDir );

#ifdef COMPLETER_SUPPORT
	QCompleter * completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	installDirLineEdit->setCompleter( completer );
#endif

	connect( openDirButton, SIGNAL( clicked() ), this, SLOT( openDir() ) );
	connect( installDirLineEdit, SIGNAL( returnPressed() ),
						this, SLOT( returnPressed() ) );
	connect( installDirLineEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( setInstallDir( const QString & ) ) );
}




bool setupWizardPageInstallDir::returnPressed( void )
{
	QString d = m_setupWizard->m_installDir + QDir::separator();
	if( !QDir( d ).exists() )
	{
		if( QMessageBox::question( NULL,
				tr( "Directory does not exist" ),
				tr( "The specified directory does not exist. "
					"Do you want to create it?" ),
				QMessageBox::Yes | QMessageBox::No,
							QMessageBox::Yes )
				==
							QMessageBox::Yes )
		{
			if( !localSystem::ensurePathExists( d ) )
			{
				QMessageBox::critical( NULL,
					tr( "Could not create directory" ),
					tr( "Could not create directory %1! "
						"Make sure you have the "
						"neccessary rights and try "
						"again!" ).arg( d ),
					QMessageBox::Ok );
				return( FALSE );
			}
		}
		else
		{
			return( FALSE );
		}
	}
	return( TRUE );
}




void setupWizardPageInstallDir::openDir( void )
{
	QString dir = QFileDialog::getExistingDirectory( NULL,
					tr( "Choose installation directory" ),
					m_setupWizard->m_installDir );
	if( !dir.isEmpty() )
	{
		setInstallDir( dir );
	}
}




void setupWizardPageInstallDir::setInstallDir( const QString & _dir )
{
	m_setupWizard->m_installDir = _dir;
	if( installDirLineEdit->text() != _dir )
	{
		installDirLineEdit->setText( _dir );
	}
}






// setupWizardPageSelectComponents
setupWizardPageSelectComponents::setupWizardPageSelectComponents(
							setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageSelectComponents()
{
	setupUi( this );
}




void setupWizardPageSelectComponents::toggleComponentClient( bool _on )
{
	m_setupWizard->m_installClient = _on;
}



void setupWizardPageSelectComponents::toggleComponentMaster( bool _on )
{
	m_setupWizard->m_installMaster = _on;
}




void setupWizardPageSelectComponents::toggleComponentDocs( bool _on )
{
	m_setupWizard->m_installDocs = _on;
}






// setupWizardPageSecurityOptions
setupWizardPageSecurityOptions::setupWizardPageSecurityOptions(
							setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageSecurityOptions()
{
	setupUi( this );

	keyImportDirLineEdit->setText( m_setupWizard->m_keyImportDir );

#ifdef COMPLETER_SUPPORT
	QCompleter * completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	keyImportDirLineEdit->setCompleter( completer );
#endif


	connect( openDirButton, SIGNAL( clicked() ), this,
						SLOT( openKeyImportDir() ) );
	connect( keyImportDirLineEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( setKeyImportDir( const QString & ) ) );
	connect( createKeysRadioButton, SIGNAL( toggled( bool ) ),
			this, SLOT( createKeysRadioButtonToggled( bool ) ) );
}




bool setupWizardPageSecurityOptions::nextPageDisabled( void )
{
	if( createKeysRadioButton->isChecked() )
	{
		m_setupWizard->m_keyImportDir.clear();
		return( FALSE );
	}
	m_setupWizard->m_keyImportDir = keyImportDirLineEdit->text();
	if( m_setupWizard->m_keyImportDir.isEmpty() )
	{
		return( TRUE );
	}
	return( !publicDSAKey( m_setupWizard->m_keyImportDir +
					QDir::separator() +
					PUBLIC_KEY_FILE_NAME ).isValid() );
}




void setupWizardPageSecurityOptions::openKeyImportDir( void )
{
	QString dir = QFileDialog::getExistingDirectory( NULL,
					tr( "Choose directory for key-import" ),
					m_setupWizard->m_keyImportDir );
	if( !dir.isEmpty() )
	{
		setKeyImportDir( dir );
	}
}




void setupWizardPageSecurityOptions::setKeyImportDir( const QString & _dir )
{
	m_setupWizard->m_keyImportDir= _dir;
	if( keyImportDirLineEdit->text() != _dir )
	{
		keyImportDirLineEdit->setText( _dir );
	}
	m_setupWizard->setNextPageDisabled( nextPageDisabled() );
}




void setupWizardPageSecurityOptions::createKeysRadioButtonToggled( bool )
{
	m_setupWizard->setNextPageDisabled( nextPageDisabled() );
}






// setupWizardPageKeyDirs
setupWizardPageKeyDirs::setupWizardPageKeyDirs( setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageKeyDirs()
{
	setupUi( this );

	pubKeyDirLineEdit->setText( m_setupWizard->m_pubKeyDir );
	privKeyDirLineEdit->setText( m_setupWizard->m_privKeyDir );
	keyExportDirLineEdit->setText( m_setupWizard->m_keyExportDir );

#ifdef COMPLETER_SUPPORT
	QCompleter * completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	pubKeyDirLineEdit->setCompleter( completer );
	completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	privKeyDirLineEdit->setCompleter( completer );
	completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	keyExportDirLineEdit->setCompleter( completer );
#endif


	connect( openPubKeyDirButton, SIGNAL( clicked() ),
					this, SLOT( openPubKeyDir() ) );
	connect( openPrivKeyDirButton, SIGNAL( clicked() ),
					this, SLOT( openPrivKeyDir() ) );
	connect( openExportKeyDirButton, SIGNAL( clicked() ),
					this, SLOT( openKeyExportDir() ) );
	connect( pubKeyDirLineEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( setPubKeyDir( const QString & ) ) );
	connect( privKeyDirLineEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( setPrivKeyDir( const QString & ) ) );
	connect( keyExportDirLineEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( setKeyExportDir( const QString & ) ) );
}




bool setupWizardPageKeyDirs::nextPageDisabled( void )
{
	newKeyDirWidgets->setVisible( m_setupWizard->m_keyImportDir.isEmpty() );
	setPubKeyDir( m_setupWizard->m_pubKeyDir.replace( DEFAULT_INSTALL_DIR,
						m_setupWizard->m_installDir ) );
	setPrivKeyDir( m_setupWizard->m_privKeyDir.replace( DEFAULT_INSTALL_DIR,
						m_setupWizard->m_installDir ) );
	return( FALSE );
}




void setupWizardPageKeyDirs::openPubKeyDir( void )
{
	QString dir = QFileDialog::getExistingDirectory( NULL,
					tr( "Choose public key directory" ),
					m_setupWizard->m_pubKeyDir );
	if( !dir.isEmpty() )
	{
		setPubKeyDir( dir );
	}
}




void setupWizardPageKeyDirs::openPrivKeyDir( void )
{
	QString dir = QFileDialog::getExistingDirectory( NULL,
					tr( "Choose private key directory" ),
					m_setupWizard->m_privKeyDir );
	if( !dir.isEmpty() )
	{
		setPrivKeyDir( dir );
	}
}




void setupWizardPageKeyDirs::openKeyExportDir( void )
{
	QString dir = QFileDialog::getExistingDirectory( NULL,
				tr( "Choose public key export directory" ),
					m_setupWizard->m_keyExportDir );
	if( !dir.isEmpty() )
	{
		setKeyExportDir( dir );
	}
}




void setupWizardPageKeyDirs::setPubKeyDir( const QString & _dir )
{
	m_setupWizard->m_pubKeyDir = _dir;
	if( pubKeyDirLineEdit->text() != _dir )
	{
		pubKeyDirLineEdit->setText( _dir );
	}
}




void setupWizardPageKeyDirs::setPrivKeyDir( const QString & _dir )
{
	m_setupWizard->m_privKeyDir = _dir;
	if( privKeyDirLineEdit->text() != _dir )
	{
		privKeyDirLineEdit->setText( _dir );
	}
}




void setupWizardPageKeyDirs::setKeyExportDir( const QString & _dir )
{
	m_setupWizard->m_keyExportDir = _dir;
	if( keyExportDirLineEdit->text() != _dir )
	{
		keyExportDirLineEdit->setText( _dir );
	}
}







#include "dialogs.moc"

