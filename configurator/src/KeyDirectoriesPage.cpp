/*
 * KeyDirectoriesPage.cpp - QWizardPage for key directory selection
 *
 * Copyright (c) 2010-2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
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

#include <QtCore/QFileInfo>

#include "KeyFileAssistant.h"
#include "KeyDirectoriesPage.h"
#include "LocalSystem.h"
#include "ui_KeyFileAssistant.h"


KeyDirectoriesPage::KeyDirectoriesPage() :
	QWizardPage(),
	m_ui( nullptr )
{
}



void KeyDirectoriesPage::setUi( Ui::KeyFileAssistant *ui )
{
	m_ui = ui;

	connect( m_ui->exportPublicKey, &QAbstractButton::clicked, this, &QWizardPage::completeChanged );
	connect( m_ui->publicKeyDir, &QLineEdit::textChanged, this, &QWizardPage::completeChanged );
	connect( m_ui->useCustomDestDir, &QAbstractButton::clicked, this, &QWizardPage::completeChanged );
	connect( m_ui->destDirEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged );
}



bool KeyDirectoriesPage::isComplete() const
{
	if( !m_ui )
	{
		return false;
	}
	if ( m_ui->modeCreateKeys->isChecked() &&
			m_ui->exportPublicKey->isChecked() )
	{
		if( m_ui->publicKeyDir->text().isEmpty() ||
				!QFileInfo( LocalSystem::Path::expand( m_ui->publicKeyDir->text() ) ).isDir() )
		{
			return false;
		}
	}
	else if( m_ui->modeImportPublicKey->isChecked() )
	{
		if( m_ui->publicKeyDir->text().isEmpty() ||
				!QFileInfo( LocalSystem::Path::expand( m_ui->publicKeyDir->text() ) ).isFile() )
		{
			return false;
		}
	}

	if( m_ui->useCustomDestDir->isChecked() )
	{
		if( m_ui->destDirEdit->text().isEmpty() ||
				!QFileInfo( LocalSystem::Path::expand( m_ui->destDirEdit->text() ) ).isDir() )
		{
			return false;
		}
	}

	return true;
}
