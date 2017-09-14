/*
 * KeyFileAssistantModePage.cpp - QWizardPage for assistant mode selection
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

#include <QButtonGroup>

#include "KeyFileAssistant.h"
#include "KeyFileAssistantModePage.h"
#include "VeyonCore.h"

#include "ui_KeyFileAssistant.h"


KeyFileAssistantModePage::KeyFileAssistantModePage() :
	QWizardPage(),
	m_modeButtonGroup( nullptr )
{
}


void KeyFileAssistantModePage::setUi( Ui::KeyFileAssistant *ui )
{
	ui->modeImportPublicKey->setChecked( true );

	m_modeButtonGroup = ui->assistantModeButtonGroup;
	connect( m_modeButtonGroup, QOverload<int>::of( &QButtonGroup::buttonClicked ), this, &QWizardPage::completeChanged );
}



bool KeyFileAssistantModePage::isComplete() const
{
	return m_modeButtonGroup && m_modeButtonGroup->checkedButton() != nullptr;
}
