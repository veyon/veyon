// SPDX-FileCopyrightText: 2011-2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>

#include "VeyonCore.h"

namespace Ui {
class AboutDialog;
}

class VEYON_CORE_EXPORT AboutDialog : public QDialog
{
	Q_OBJECT
public:
	explicit AboutDialog(QWidget* parent);
	~AboutDialog() override;

private Q_SLOTS:
	void openDonationWebsite();

private:
	Ui::AboutDialog* ui;
};
