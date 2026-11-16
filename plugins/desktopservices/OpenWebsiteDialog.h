// SPDX-FileCopyrightText: 2019-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

namespace Ui {
class OpenWebsiteDialog;
}

class OpenWebsiteDialog : public QDialog
{
	Q_OBJECT
public:
	explicit OpenWebsiteDialog(QWidget* parent);
	~OpenWebsiteDialog() override;

	const QString& website() const
	{
		return m_website;
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

	Ui::OpenWebsiteDialog* ui;
	QString m_website;
	bool m_remember;
	QString m_presetName;
};
