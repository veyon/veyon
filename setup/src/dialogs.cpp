/*
 * dialogs.cpp - implementation of dialogs
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QProcess>
#include <QtGui/QCloseEvent>
#include <QtGui/QCompleter>
#include <QtGui/QDirModel>
#include <QtGui/QFileDialog>
#include <QtGui/QLayout>
#include <QtGui/QMessageBox>
#include <QtXml/QtXml>

#ifdef BUILD_WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#include "dialogs.h"
#include "local_system.h"
#include "dsa_key.h"



const QString PUBLIC_KEY_FILE_NAME = "italc_dsa_key.pub";
const QString KEEP_SETTINGS = ":keep-settings:";
const QString DEFAULT_INSTALL_DIR =
#ifdef BUILD_WIN32
									"c:\\italc"
#else
									"/opt/italc"
#endif
												;


setupWizard::setupWizard( const QString & installDir ) :
	QDialog(),
	Ui::wizard(),
	m_installDir( installDir ),
	m_keyImportDir( QDir::homePath().replace( "/", QDir::separator() ) +
							QDir::separator() ),
	m_keyExportDir( m_keyImportDir ),
	m_pubKeyDir( localSystem::publicKeyPath( ISD::RoleTeacher, TRUE ) ),
	m_privKeyDir( localSystem::privateKeyPath( ISD::RoleTeacher, TRUE ) ),
	m_saveInstallSettings( FALSE ),
	m_widgetStack(),
	m_idx( 0 ),
	m_closeOk( FALSE )
{
	setupUi( this );

	connect( backButton, SIGNAL( clicked() ), this, SLOT( back() ) );
	connect( nextButton, SIGNAL( clicked() ), this, SLOT( next() ) );

	nextButton->setFocus();


	QHBoxLayout * hboxLayout = new QHBoxLayout( contentWidget );
	hboxLayout->setMargin( 0 );
	hboxLayout->setSpacing( 0 );

	m_widgetStack << new setupWizardPageWelcome( this );
	m_widgetStack << new setupWizardPageSecurityOptions( this );
	m_widgetStack << new setupWizardPageKeyDirs( this );
	m_widgetStack << new setupWizardPageFinished( this );

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




void setupWizard::loadSettings( const QString & _install_settings )
{
	QFile in( _install_settings );
	in.open( QFile::ReadOnly );
	QDomDocument doc;
	doc.setContent( in.readAll() );
	QDomElement root = doc.documentElement();
	m_installDir = root.attribute( "installdir" );
	m_keyImportDir = root.attribute( "keyimportdir" );
	m_keyExportDir = root.attribute( "keyexportdir" );
	m_pubKeyDir = root.attribute( "pubkeydir" );
	m_privKeyDir = root.attribute( "privkeydir" );
}




int setupWizard::askOverwrite( const QString & _file, bool _all )
{
	return QMessageBox::question( window(), tr( "Confirm overwrite" ),
			tr( "Do you want to overwrite %1?" ).arg( _file ),
			QMessageBox::Yes | QMessageBox::No |
			( _all ?
				( QMessageBox::YesToAll | QMessageBox::NoToAll )
						: QMessageBox::NoButton )
							, QMessageBox::Yes );
}




#ifdef BUILD_WIN32
static const QString _exe_ext = ".exe";
#else
static const QString _exe_ext = "";
#endif

void setupWizard::doInstallation( bool _quiet )
{
	const QString & d = m_installDir + QDir::separator();
	QStringList quiet_opt;
	if( _quiet )
	{
		quiet_opt << "-quiet";
	}

	QSettings settings( QSettings::SystemScope, "iTALC Solutions", "iTALC" );
	settings.setValue( "settings/LogLevel", 6 );
	
#ifdef BUILD_WIN32
	// disable firewall for ICA-process
	HKEY hKey; 
	const QString val = d + "ica.exe:*:Enabled:iTALC Client Application (ICA)";
	RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\"
				"Parameters\\FirewallPolicy\\DomainProfile\\"
				"AuthorizedApplications\\List",
                    NULL, KEY_SET_VALUE, &hKey );

	RegSetValueEx( hKey, QString( d + "ica.exe" ).toUtf8().constData(),
			0, REG_SZ,
			(LPBYTE) QString( val ).toUtf8().constData(),
			val.size()+1 );
	RegCloseKey( hKey );
	RegOpenKeyEx( HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\"
				"Parameters\\FirewallPolicy\\StandardProfile\\"
				"AuthorizedApplications\\List",
                    NULL, KEY_SET_VALUE, &hKey );

	RegSetValueEx( hKey, QString( d + "ica.exe" ).toUtf8().constData(),
			0, REG_SZ,
			(LPBYTE) QString( val ).toUtf8().constData(),
			val.size()+1 );
	RegCloseKey( hKey );
#endif

	const QString add = QDir::separator() + QString( "key" );
	const QString add2 = QDir::separator() + PUBLIC_KEY_FILE_NAME;
	localSystem::setPrivateKeyPath( m_privKeyDir + add,
						ISD::RoleTeacher );
	localSystem::setPublicKeyPath( m_pubKeyDir + add,
						ISD::RoleTeacher );
	if( m_keyImportDir.isEmpty() )
	{
		QProcess::execute( d + "ica",
					QStringList() << "-createkeypair"
						<< ( m_privKeyDir + add )
						<< ( m_pubKeyDir + add ) );
		if( !_quiet || !QFileInfo( m_keyExportDir+add2 ).exists() ||
			askOverwrite( m_keyExportDir+add2 ) ==
							QMessageBox::Yes )
		{
			QFile( m_keyExportDir+add2 ).remove();
			QFile( m_pubKeyDir+add ).copy( m_keyExportDir + add2 );
		}
	}
	else if( m_keyImportDir != KEEP_SETTINGS )
	{
		publicDSAKey( m_keyImportDir + add2 ).
					save( m_pubKeyDir + add );
	}

	// make public key read-only
	QFile::setPermissions( m_pubKeyDir + add,
				QFile::ReadOwner | QFile::ReadUser |
				QFile::ReadGroup | QFile::ReadOther );
}




void setupWizard::reject()
{
	if( QMessageBox::question( window(),
			tr( "Cancel setup" ),
			tr( "Are you sure want to quit setup? iTALC is not "
				"set up completely yet!" ),
			QMessageBox::Yes | QMessageBox::No,
						QMessageBox::Yes )
			==
						QMessageBox::Yes )
	{
		m_closeOk = TRUE;
		QDialog::reject();
	}
}




void setupWizard::back()
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




void setupWizard::next()
{
	if( !m_widgetStack[m_idx]->nextPage() )
	{
		return;
	}
	if( m_idx+3 == m_widgetStack.size() && m_keyImportDir == KEEP_SETTINGS )
	{
		// skip key-directory-page when user chose to keep keys
		doInstallation();
		m_widgetStack[m_idx]->hide();
		++m_idx;
	}
	else if( m_idx+2 == m_widgetStack.size() )
	{
		doInstallation();
	}
	else if( m_idx+1 == m_widgetStack.size() )
	{
		if( m_saveInstallSettings )
		{
			QDomDocument doc( "italc-installation-settings" );
			QDomElement root = doc.createElement( "settings" );
			root.setAttribute( "installdir", m_installDir );
			root.setAttribute( "keyimportdir", m_keyImportDir );
			root.setAttribute( "keyexportdir", m_keyExportDir );
			root.setAttribute( "pubkeydir", m_pubKeyDir );
			root.setAttribute( "privkeydir", m_privKeyDir );
			doc.appendChild( root );
			QFile out( "installsettings.xml" );
			out.open( QIODevice::WriteOnly | QIODevice::Truncate );
			QString xml = "<?xml version=\"1.0\"?>\n" +
							doc.toString( 4 );
			out.write( xml.toUtf8().constData(), xml.length() );
		}
		accept();
		return;
	}

	m_widgetStack[m_idx]->hide();
	m_widgetStack[++m_idx]->show();
	if( m_widgetStack[m_idx]->nextPageDisabled() )
	{
		setNextPageDisabled( TRUE );
	}
	if( m_idx+2 == m_widgetStack.size() )
	{
		nextButton->setText( tr( "Finish" ) );
	}
	else if( m_idx+1 == m_widgetStack.size() )
	{
		cancelButton->setDisabled( TRUE );
		backButton->setDisabled( TRUE );
		nextButton->setText( tr( "Quit" ) );
	}
	nextButton->setFocus();
}




void setupWizard::closeEvent( QCloseEvent * _ev )
{
	if( m_closeOk )
	{
		QDialog::closeEvent( _ev );
	}
	else
	{
		reject();
		if( !m_closeOk )
		{
			_ev->ignore();
		}
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





// setupWizardPageSecurityOptions
setupWizardPageSecurityOptions::setupWizardPageSecurityOptions(
							setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageSecurityOptions()
{
	setupUi( this );

	keyImportDirLineEdit->setText( m_setupWizard->m_keyImportDir );

	QCompleter * completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	keyImportDirLineEdit->setCompleter( completer );


	connect( openDirButton, SIGNAL( clicked() ), this,
						SLOT( openKeyImportDir() ) );
	connect( keyImportDirLineEdit, SIGNAL( textChanged( const QString & ) ),
			this, SLOT( setKeyImportDir( const QString & ) ) );
	connect( createKeysRadioButton, SIGNAL( toggled( bool ) ),
			this, SLOT( createKeysRadioButtonToggled( bool ) ) );
	connect( keepKeysRadioButton, SIGNAL( toggled( bool ) ),
			this, SLOT( keepKeysRadioButtonToggled( bool ) ) );
}




bool setupWizardPageSecurityOptions::nextPageDisabled()
{
	if( createKeysRadioButton->isChecked() )
	{
		m_setupWizard->m_keyImportDir.clear();
		return FALSE;
	}
	if( keepKeysRadioButton->isChecked() )
	{
		m_setupWizard->m_keyImportDir = KEEP_SETTINGS;
		return FALSE;
	}
	m_setupWizard->m_keyImportDir = keyImportDirLineEdit->text();
	if( m_setupWizard->m_keyImportDir.isEmpty() )
	{
		return TRUE;
	}
	return !publicDSAKey( m_setupWizard->m_keyImportDir +
					QDir::separator() +
					PUBLIC_KEY_FILE_NAME ).isValid();
}




void setupWizardPageSecurityOptions::openKeyImportDir()
{
	QString dir = QFileDialog::getExistingDirectory( window(),
					tr( "Choose directory for key import" ),
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




void setupWizardPageSecurityOptions::keepKeysRadioButtonToggled( bool )
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

	QCompleter * completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	pubKeyDirLineEdit->setCompleter( completer );
	completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	privKeyDirLineEdit->setCompleter( completer );
	completer = new QCompleter( this );
	completer->setModel( new QDirModel( completer ) );
 	keyExportDirLineEdit->setCompleter( completer );


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




bool setupWizardPageKeyDirs::nextPageDisabled()
{
	newKeyDirWidgets->setVisible( m_setupWizard->m_keyImportDir.isEmpty() );
	setPubKeyDir( m_setupWizard->m_pubKeyDir.replace( DEFAULT_INSTALL_DIR,
						m_setupWizard->m_installDir ) );
	setPrivKeyDir( m_setupWizard->m_privKeyDir.replace( DEFAULT_INSTALL_DIR,
						m_setupWizard->m_installDir ) );
	return FALSE;
}




void setupWizardPageKeyDirs::openPubKeyDir()
{
	QString dir = QFileDialog::getExistingDirectory( window(),
					tr( "Choose public key directory" ),
					m_setupWizard->m_pubKeyDir );
	if( !dir.isEmpty() )
	{
		setPubKeyDir( dir );
	}
}




void setupWizardPageKeyDirs::openPrivKeyDir()
{
	QString dir = QFileDialog::getExistingDirectory( window(),
					tr( "Choose private key directory" ),
					m_setupWizard->m_privKeyDir );
	if( !dir.isEmpty() )
	{
		setPrivKeyDir( dir );
	}
}




void setupWizardPageKeyDirs::openKeyExportDir()
{
	QString dir = QFileDialog::getExistingDirectory( window(),
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





setupWizardPageFinished::setupWizardPageFinished( setupWizard * _wiz ) :
	setupWizardPage( _wiz ),
	Ui::pageFinished()
{
	setupUi( this );
	connect( saveInstallSettings, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleSaveInstallSettings( bool ) ) );
}




void setupWizardPageFinished::toggleSaveInstallSettings( bool _on )
{
	m_setupWizard->m_saveInstallSettings = _on;
}






#include "dialogs.moc"

