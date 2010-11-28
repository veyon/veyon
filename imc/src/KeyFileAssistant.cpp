/*
 * KeyFileAssistant.cpp - a wizard assisting in managing key files
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>
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

#include <QtCore/QDir>
#include <QtGui/QMessageBox>

#include "KeyFileAssistant.h"
#include "DsaKey.h"
#include "FileSystemBrowser.h"
#include "ImcCore.h"
#include "ItalcConfiguration.h"
#include "ItalcCore.h"
#include "LocalSystem.h"
#include "Logger.h"
#include "ui_KeyFileAssistant.h"


KeyFileAssistant::KeyFileAssistant() :
	QWizard(),
	m_ui( new Ui::KeyFileAssistant )
{
	m_ui->setupUi( this );

	m_ui->assistantModePage->setUi( m_ui );
	m_ui->directoriesPage->setUi( m_ui );

	// init destination directory line edit
	QDir d( LocalSystem::Path::expand( ItalcCore::config->privateKeyBaseDir() ) );
	d.cdUp();
	m_ui->destDirEdit->setText( QDTNS( d.absolutePath() ) );

	connect( m_ui->openDestDir, SIGNAL( clicked() ),
				this, SLOT( openDestDir() ) );

	connect( m_ui->openPubKeyDir, SIGNAL( clicked() ),
				this, SLOT( openPubKeyDir() ) );
}



KeyFileAssistant::~KeyFileAssistant()
{
}




void KeyFileAssistant::openPubKeyDir()
{
	if( m_ui->modeCreateKeys->isChecked() )
	{
		FileSystemBrowser b( FileSystemBrowser::ExistingDirectory );
		b.setShrinkPath( false );
		b.exec( m_ui->publicKeyDir,
				tr( "Select directory in which to export the public key" ) );
	}
	else if( m_ui->modeImportPublicKey->isChecked() )
	{
		QString origPath = m_ui->publicKeyDir->text();

		FileSystemBrowser b( FileSystemBrowser::ExistingFile );
		b.setShrinkPath( false );
		b.exec( m_ui->publicKeyDir,
				tr( "Select directory in which to export the public key" ),
				tr( "Key files (*.key.txt)" ) );

		if( !PublicDSAKey( m_ui->publicKeyDir->text() ).isValid() )
		{
			m_ui->publicKeyDir->setText( origPath );
			QMessageBox::critical( this, tr( "Invalid public key" ),
					tr( "The selected file does not contain a valid public "
						"iTALC access key!" ) );
		}
	}
}




void KeyFileAssistant::openDestDir()
{
	FileSystemBrowser b( FileSystemBrowser::ExistingDirectory );
	b.setShrinkPath( false );
	b.exec( m_ui->destDirEdit, tr( "Select destination directory" ) );
}




void KeyFileAssistant::accept()
{
	ItalcCore::UserRole role =
			static_cast<ItalcCore::UserRole>(
					m_ui->userRole->currentIndex() + ItalcCore::RoleTeacher );

	QString destDir;
	if( m_ui->useCustomDestDir->isChecked() ||
				// trap the case public and private key path are equal
				LocalSystem::Path::publicKeyPath( role ) ==
					LocalSystem::Path::privateKeyPath( role ) )
	{
		destDir = m_ui->destDirEdit->text();
	}

	if( m_ui->modeCreateKeys->isChecked() )
	{
		if( ImcCore::createKeyPair( role, destDir ) )
		{
			if( m_ui->exportPublicKey->isChecked() )
			{
				QFile src( LocalSystem::Path::publicKeyPath( role, destDir ) );
				QFile dst( QDTNS( m_ui->publicKeyDir->text() +
										"/italc_public_key.key.txt" ) );
				if( dst.exists() )
				{
					dst.setPermissions( QFile::WriteOwner );
					if( !dst.remove() )
					{
						QMessageBox::critical( this, tr( "Access key creation" ),
							tr( "Could not remove previously existing file %1." ).
								arg( dst.fileName() ) );
						return;
					}
				}
				if( !src.copy( dst.fileName() ) )
				{
					QMessageBox::critical( this, tr( "Access key creation" ),
							tr( "Failed exporting public access key from %1 to %2." ).
								arg( src.fileName() ).arg( dst.fileName() ) );
					return;
				}
			}
			QMessageBox::information( this, tr( "Access key creation" ),
				tr( "Access keys were created and written successfully to %1 and %2." ).
					arg( LocalSystem::Path::privateKeyPath( role, destDir ) ).
					arg( LocalSystem::Path::publicKeyPath( role, destDir ) ) );
		}
		else
		{
			QMessageBox::critical( this, tr( "Access key creation" ),
					tr( "An error occured while creating the access keys. "
						"You probably are not permitted to write to the "
						"selected directories." ) );
			return;
		}

	}
	else if( m_ui->modeImportPublicKey->isChecked() )
	{
		if( !ImcCore::importPublicKey( role, m_ui->publicKeyDir->text(), destDir ) )
		{
			QMessageBox::critical( this, tr( "Public key import" ),
					tr( "An error occured while importing the public access "
						"key. You probably are not permitted to read the "
						"source key or to write the destination file." ) );
			return;
		}
		QMessageBox::information( this, tr( "Public key import" ),
				tr( "The public key was successfully imported to %1." ).
					arg( LocalSystem::Path::publicKeyPath( role, destDir ) ) );
	}

	QWizard::accept();
}

