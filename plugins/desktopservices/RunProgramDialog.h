/*
 * RunProgramDialog.h - declaration of class RunProgramDialog
 *
 * Copyright (c) 2004-2019 Tobias Junghans <tobydox@veyon.io>
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

#pragma once

#include <QDialog>

namespace Ui { class RunProgramDialog; }

class RunProgramDialog : public QDialog
{
	Q_OBJECT
public:
	RunProgramDialog( QWidget *parent );
	~RunProgramDialog() override;

	const QString& programs() const
	{
		return m_programs;
	}

	bool remember() const
	{
		return m_remember;
	}

	const QString& presetName() const
	{
		return m_presetName;
	}

private:
	void validate();
	void accept() override;

	Ui::RunProgramDialog* ui;
	QString m_programs;
	bool m_remember;
	QString m_presetName;

} ;
