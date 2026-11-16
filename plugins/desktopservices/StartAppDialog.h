// SPDX-FileCopyrightText: 2004-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class StartAppDialog;
}

class StartAppDialog : public QDialog
{
	Q_OBJECT
public:
	explicit StartAppDialog(QWidget* parent);
	~StartAppDialog() override;

	const QString& apps() const
	{
		return m_apps;
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

	Ui::StartAppDialog* ui;
	QString m_apps;
	bool m_remember;
	QString m_presetName;
};
